# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
import helpers
import os
import shutil

WORKING = 'working'
OLD = 'old'
NEW = 'new'
LIST = 'list'

class WorkingFolderManager:
    def __init__(self, parent_folder):
        self.working_folder = helpers.get_combined_path(parent_folder, WORKING)
        self.subfolder_old = helpers.get_combined_path(self.working_folder, OLD)
        self.subfolder_new = helpers.get_combined_path(self.working_folder, NEW)
        self.list_file = helpers.get_combined_path(self.working_folder, LIST)

        if os.path.exists(self.working_folder):
            self.dispose()

        helpers.create_folder(self.subfolder_old)
        helpers.create_folder(self.subfolder_new)

    def dispose(self):
        shutil.rmtree(self.working_folder)
