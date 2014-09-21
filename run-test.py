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
        
    def compareFunctions(self, actual, expected):
        self.assertEqual(actual["name"], expected["name"])
        self.assertEqual(actual["qualified_name"], expected["qualified_name"])
        self.assertEqual(actual["qualified_name"].split("::")[-1], actual["name"])
        self.assertEqual(actual["return"], expected["return"])
        self.assertEqual(actual["params"], expected["params"])
        self.assertEqual(actual["deleted"], expected["deleted"])
        
    def compareClases(self, actual, expected):
        self.assertEqual(actual["name"], expected["name"])
        self.assertEqual(actual["qualified_name"], expected["qualified_name"])
        self.assertEqual(actual["qualified_name"].split("::")[-1], actual["name"])
        self.assertEqual(actual["base_class"], expected["base_class"])
        for base in actual["base_class"]:
            self.assertIn(base["access"], {"private", "protected", "public"})
        
        self.assertEqual(len(actual["methods"]), len(expected["methods"]))
        for i in xrange(len(actual["methods"])):
            act_method = actual["methods"][i]
            exp_method = expected["methods"][i]
            self.compareFunctions(act_method, exp_method)
            self.assertEqual(act_method["pure"], exp_method["pure"])
            self.assertEqual(act_method["virtual"], exp_method["virtual"])
            self.assertEqual(act_method["const"], exp_method["const"])
            self.assertEqual(act_method["access"], exp_method["access"])
            self.assertIn(act_method["access"], {"private", "protected", "public"})

    def test_namespaces(self):
        self.assertEqual(self.actual_output["namespaces"], self.expected_output["namespaces"])
        
    def test_classes(self):
        self.assertEqual(len(self.actual_output["classes"]), len(self.expected_output["classes"]))
        for i in xrange(len(self.actual_output["classes"])):
            self.compareClases(self.actual_output["classes"][i], self.expected_output["classes"][i])
            
    def test_functions(self):
        self.assertEqual(len(self.actual_output["functions"]), len(self.expected_output["functions"]))
        for i in xrange(len(self.actual_output["functions"])):
            self.compareFunctions(self.actual_output["functions"][i], self.expected_output["functions"][i])

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
    