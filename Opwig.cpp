// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceLocation.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include <unordered_map>
#include <iostream>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...");

//

struct OPClass {
    std::string name;

    friend std::ostream& operator<< (std::ostream& os, const OPClass& self) {
        os << "\"name\": \"" << self.name << "\"\n";
        return os;
    }
};
struct OPFunction {
    std::string name;

    friend std::ostream& operator<< (std::ostream& os, const OPFunction& self) {
        os << "\"name\": \"" << self.name << "\"\n";
        return os;
    }
};

std::unordered_map<std::string, OPClass> opclasses;
std::unordered_map<std::string, OPFunction> opfunctions;

DeclarationMatcher RecordMatcher = recordDecl(isDefinition()).bind("recordMatch");
DeclarationMatcher FunctionMatcher = functionDecl().bind("functionMatch");

class RecordPrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto Item = Result.Nodes.getDeclAs<CXXRecordDecl>("recordMatch");
        if (Item) {
            auto& cl = opclasses[Item->getQualifiedNameAsString()];
            cl.name = Item->getNameAsString();
        }
    }
};

class FunctionPrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto Item = Result.Nodes.getDeclAs<FunctionDecl>("functionMatch");
        if (Item) {
            auto& fn = opfunctions[Item->getQualifiedNameAsString()];
            fn.name = Item->getNameAsString();
        }
    }
};


//

int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  RecordPrinter RePrinter;
  FunctionPrinter FunPrinter;
  MatchFinder Finder;
  Finder.addMatcher(RecordMatcher, &RePrinter);
  Finder.addMatcher(FunctionMatcher, &FunPrinter);

  int result = Tool.run(newFrontendActionFactory(&Finder).get());
  if (result == 0) {
      bool has_prev = false;
      std::cout << "{\n\"classes\": {";
      for (const auto& cl : opclasses) {
          if (has_prev)
              std::cout << ",";
          std::cout << "\n\"" << cl.first << "\": {\n";
          std::cout << cl.second;
          std::cout << "}";
          has_prev = true;
      }

      has_prev = false;
      std::cout << "},\n\"functions\": {";
      for (const auto& fn : opfunctions) {
          if (has_prev)
              std::cout << ",";
          std::cout << "\n\"" << fn.first << "\": {\n";
          std::cout << fn.second;
          std::cout << "}";
          has_prev = true;
      }
      std::cout << "}\n}\n";
  }
  return result;
}
