// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jacobsa@google.com (Aaron Jacobs)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// The main Google JS Test tool. Executes each JS file it's given, in order,
// then runs any tests that were registered in the process.
//
// Input files should be topologically sorted. That is, if you have
// foo_test.js, which tests foo.js, which depends on bar.js and baz.js, the
// input should look like this:
//
//     --js_files=\
//         bar.js,\
//         baz.js,\
//         foo.js,\
//         foo_test.js
//
// (The relative order of bar.js and baz.js does not matter in this example.)
//
// Dependencies common to all gjstest tests (e.g. built-in matchers and the
// mocking framework) are added automatically, and should not be specified.

#include <iostream>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/integral_types.h"
#include "base/logging.h"
#include "file/file_utils.h"
#include "gjstest/internal/cpp/get_builtin_scripts.h"
#include "gjstest/internal/cpp/run_tests.h"
#include "gjstest/internal/proto/named_scripts.pb.h"
#include "strings/strutil.h"

DEFINE_string(js_files, "",
              "The list of JS files to execute, comma separated.");

DEFINE_string(xml_output_file, "", "An XML file to write results to.");
DEFINE_string(filter, "", "Regular expression for test names to run.");

namespace gjstest {

// Attempt to read in all of the built-in and user-specified scripts.
static bool GetScripts(
    NamedScripts* scripts,
    string* error) {
  // First attempt to get the built-in ones.
  if (!GetBuiltinScripts(scripts, error)) {
    return false;
  }

  // Load the paths specified by the user.
  vector<string> paths;
  SplitStringUsing(FLAGS_js_files, ",", &paths);

  for (uint32 i = 0; i < paths.size(); ++i) {
    const string& path = paths[i];

    NamedScript* script = scripts->add_script();
    script->set_name(Basename(path));
    script->set_source(ReadFileOrDie(path));
  }

  return true;
}

static bool Run() {
  // Attempt to load the appropriate scripts.
  NamedScripts scripts;
  string error;
  if (!GetScripts(&scripts, &error)) {
    LOG(ERROR) << "Failed to load scripts: " << error;
    return false;
  }

  // Run any tests registered.
  string output;
  string xml;
  const bool success = RunTests(scripts, FLAGS_filter, &output, &xml);

  // Log the output.
  //
  // TODO(jacobsa): Use a different severity?
  std::cout << output;

  // Write out the XML file to the appropriate place.
  if (!FLAGS_xml_output_file.empty()) {
    WriteStringToFileOrDie(xml, FLAGS_xml_output_file);
  }

  return success;
}

}  // namespace gjstest

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  return gjstest::Run() ? 0 : 1;
}