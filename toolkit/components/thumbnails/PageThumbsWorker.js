/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * A worker dedicated for the I/O component of PageThumbs storage.
 *
 * Do not rely on the API of this worker. In a future version, it might be
 * fully replaced by a OS.File global I/O worker.
 */

"use strict";

importScripts("resource://gre/modules/osfile.jsm");

let File = OS.File;
let Type = OS.Shared.Type;

/**
 * Communications with the controller.
 *
 * Accepts messages:
 * {fun:function_name, args:array_of_arguments_or_null}
 *
 * Sends messages:
 * {ok: result} / {fail: serialized_form_of_OS.File.Error}
 */
self.onmessage = function onmessage(msg) {
  let data = msg.data;
  let id = data.id;
  let result;
  if (!(data.fun in Agent)) {
    throw new Error("Cannot find method " + data.fun);
  }
  try {
    result = Agent[data.fun].apply(Agent, data.args);
  } catch (ex if ex instanceof StopIteration) {
    // StopIteration cannot be serialized automatically
    self.postMessage({StopIteration: true, id: id});
    return;
  } catch (ex if ex instanceof OS.File.Error) {
    // Instances of OS.File.Error know how to serialize themselves
    // (deserialization ensures that we end up with OS-specific
    // instances of |OS.File.Error|)
    self.postMessage({fail: OS.File.Error.toMsg(ex), id:id});
    return;
  }
  // Other exceptions do not, and should be propagated through DOM's
  // built-in mechanism for uncaught errors, although this mechanism
  // may lose interesting information.
  self.postMessage({ok: result, id:id});
};


let Agent = {
  // Checks if the specified file exists and has an age less than as
  // specifed (in seconds).
  isFileRecent: function Agent_isFileRecent(path, maxAge) {
    try {
      let stat = OS.File.stat(path);
      let maxDate = new Date();
      maxDate.setSeconds(maxDate.getSeconds() - maxAge);
      return stat.lastModificationDate > maxDate;
    } catch (ex if ex instanceof OS.File.Error) {
      // file doesn't exist (or can't be stat'd) - must be stale.
      return false;
    }
  },

  remove: function Agent_removeFile(path) {
    try {
      OS.File.remove(path);
      return true;
    } catch (e) {
      return false;
    }
  },

  expireFilesInDirectory:
  function Agent_expireFilesInDirectory(path, filesToKeep, minChunkSize) {
    let entries = this.getFileEntriesInDirectory(path, filesToKeep);
    let limit = Math.max(minChunkSize, Math.round(entries.length / 2));

    for (let entry of entries) {
      this.remove(entry.path);

      // Check if we reached the limit of files to remove.
      if (--limit <= 0) {
        break;
      }
    }

    return true;
  },

  getFileEntriesInDirectory:
  function Agent_getFileEntriesInDirectory(path, skipFiles) {
    let iter = new OS.File.DirectoryIterator(path);
    if (!iter.exists()) {
      return [];
    }

    let skip = new Set(skipFiles);

    return [entry
            for (entry in iter)
            if (!entry.isDir && !entry.isSymLink && !skip.has(entry.name))];
  },

  moveOrDeleteAllThumbnails:
  function Agent_moveOrDeleteAllThumbnails(pathFrom, pathTo) {
    OS.File.makeDir(pathTo, {ignoreExisting: true});
    if (pathFrom == pathTo) {
      return true;
    }
    let iter = new OS.File.DirectoryIterator(pathFrom);
    if (iter.exists()) {
      for (let entry in iter) {
        if (entry.isDir || entry.isSymLink) {
          continue;
        }


        let from = OS.Path.join(pathFrom, entry.name);
        let to = OS.Path.join(pathTo, entry.name);

        try {
          OS.File.move(from, to, {noOverwrite: true, noCopy: true});
        } catch (e) {
          OS.File.remove(from);
        }
      }
    }
    iter.close();

    try {
      OS.File.removeEmptyDir(pathFrom);
    } catch (e) {
      // This could fail if there's something in
      // the folder we're not permitted to remove.
    }

    return true;
  },

  writeAtomic: function Agent_writeAtomic(path, buffer, options) {
    return File.writeAtomic(path,
      buffer,
      options);
  },

  makeDir: function Agent_makeDir(path, options) {
    return File.makeDir(path, options);
  },

  copy: function Agent_copy(source, dest) {
    return File.copy(source, dest);
  },

  wipe: function Agent_wipe(path) {
    let iterator = new File.DirectoryIterator(path);
    try {
      for (let entry in iterator) {
        try {
          File.remove(entry.path);
        } catch (ex) {
          // If a file cannot be removed, we should still continue.
          // This can happen at least for any of the following reasons:
          // - access denied;
          // - file has been removed recently during a previous wipe
          //  and the file system has not flushed that yet (yes, this
          //  can happen under Windows);
          // - file has been removed by the user or another process.
        }
      }
    } finally {
      iterator.close();
    }
  },

  touchIfExists: function Agent_touchIfExists(path) {
    // No OS.File way to update the modification date of the file (bug 905509)
    // so we open it for reading and writing, read 1 byte from the start of
    // the file then write that byte back out.
    // (Sadly it's impossible to use nsIFile here as we have no access to
    // |Components|)
    if (!File.exists(path)) {
      return false;
    }
    let file = OS.File.open(path, { read: true, write: true });
    try {
      file.setPosition(0); // docs aren't clear on initial position, so seek to 0.
      let byte = file.read(1);
      file.setPosition(0);
      file.write(byte);
    } finally {
      file.close();
    }
    return true;
  },

};

