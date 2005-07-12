# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# Contributor(s): Tiago R. Mello <timello@async.com.br>
#

use strict;

package Bugzilla::Classification;

use Bugzilla;
use Bugzilla::Util;

###############################
####    Initialization     ####
###############################

use constant DB_COLUMNS => qw(
    classifications.id
    classifications.name
    classifications.description
);

our $columns = join(", ", DB_COLUMNS);

###############################
####       Methods         ####
###############################

sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
    my $self = {};
    bless($self, $class);
    return $self->_init(@_);
}

sub _init {
    my $self = shift;
    my ($param) = @_;
    my $dbh = Bugzilla->dbh;

    my $id = $param unless (ref $param eq 'HASH');
    my $classification;

    if (defined $id && detaint_natural($id)) {

        $classification = $dbh->selectrow_hashref(qq{
            SELECT $columns FROM classifications
            WHERE id = ?}, undef, $id);

    } elsif (defined $param->{'name'}) {

        trick_taint($param->{'name'});
        $classification = $dbh->selectrow_hashref(qq{
            SELECT $columns FROM classifications
            WHERE name = ?}, undef, $param->{'name'});
    } else {
        ThrowCodeError('bad_arg',
            {argument => 'param',
             function => 'Bugzilla::Classification::_init'});
    }

    return undef unless (defined $classification);

    foreach my $field (keys %$classification) {
        $self->{$field} = $classification->{$field};
    }
    return $self;
}

sub product_count {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{'product_count'}) {
        $self->{'product_count'} = $dbh->selectrow_array(q{
            SELECT COUNT(*) FROM products
            WHERE classification_id = ?}, undef, $self->id);
    }
    return $self->{'product_count'};
}

###############################
####      Accessors        ####
###############################

sub id          { return $_[0]->{'id'};          }
sub name        { return $_[0]->{'name'};        }
sub description { return $_[0]->{'description'}; }

###############################
####      Subroutines      ####
###############################

sub get_all_classifications () {
    my $dbh = Bugzilla->dbh;

    my $ids = $dbh->selectcol_arrayref(q{
        SELECT id FROM classifications});

    my $classifications;
    foreach my $id (@$ids) {
        $classifications->{$id} = new Bugzilla::Classification($id);
    }
    return $classifications;
}

1;

__END__

=head1 NAME

Bugzilla::Classification - Bugzilla classification class.

=head1 SYNOPSIS

    use Bugzilla::Classification;

    my $classification = new Bugzilla::Classification(1);
    my $classification = new Bugzilla::Classification({name => 'Acme'});

    my $id = $classification->id;
    my $name = $classification->name;
    my $description = $classification->description;
    my $product_count = $classification->product_count;

    my $hash_ref = Bugzilla::Classification::get_all_classifications();
    my $classification = $hash_ref->{1};

=head1 DESCRIPTION

Classification.pm represents a Classification object.

A Classification is a higher-level grouping of Bugzilla Products.

=head1 METHODS

=over

=item C<new($param)>

 Description: The constructor is used to load an existing
              classification by passing a classification
              id or classification name using a hash.

 Params:      $param - If you pass an integer, the integer is the
                      classification_id from the database that we
                      want to read in. If you pass in a hash with
                      'name' key, then the value of the name key
                      is the name of a classification from the DB.

 Returns:     A Bugzilla::Classification object.

=item C<product_count()>

 Description: Returns the total number of products that belong to
              the classification.

 Params:      none.

 Returns:     Integer - The total of products inside the classification.

=back

=head1 SUBROUTINES

=over

=item C<get_all_classifications()>

 Description: Returns all Bugzilla classifications.

 Params:      none.

 Returns:     A hash with classification id as key and
              Bugzilla::Classification object as value.

=back

=cut
