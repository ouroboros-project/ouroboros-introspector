import argparse, os, json, subprocess, unittest

parser = argparse.ArgumentParser()
parser.add_argument("introspector")
parser.add_argument("input")
parser.add_argument("output")
args = parser.parse_args()

class IntrospectorTestCase():
    def setUp(self):
        global args
        self.actual_output = json.loads(subprocess.check_output([
            args.introspector,
            self.input_file,
            "--"
        ]))
        with open(output_file) as of:
            self.expected_output = json.load(of)

    def tearDown(self):
        self.actual_output = None
        self.expected_output = None

    def test_equals(self):
        self.assertEqual(self.actual_output, self.expected_output)

input_suite = unittest.TestSuite()

for (dir, _, files) in os.walk(args.input):
    for file in files:
        input_file = os.path.join(dir, file)
        output_file = os.path.join(dir.replace(args.input, args.output, 1), file + ".json")
        
        newclass = type(input_file, (IntrospectorTestCase, unittest.TestCase), {})
        newclass.input_file = input_file
        newclass.output_file = output_file
        
        file_suite = unittest.TestLoader().loadTestsFromTestCase(newclass)        
        input_suite.addTest(file_suite)

if __name__ == '__main__':
    unittest.main(argv=["asdf"])
    