// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceLocation.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include <set>
#include <unordered_map>
#include <iostream>
#include <sstream>

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

std::string JsonEscape(const std::string& s) {
    return "\"" + s + "\"";
}

std::string JsonEscape(bool b) {
    return b ? "true" : "false";
}

//

static std::string DumpToJson(const CXXRecordDecl* cl) {
    std::stringstream ss;
    ss << "\n{\n";
    ss << "\"qualified_name\": " << JsonEscape(cl->getQualifiedNameAsString()) << ",\n";
    ss << "\"name\": " << JsonEscape(cl->getNameAsString()) << "\n";
    ss << "}";
    return ss.str();
}

static std::string DumpToJson(const FunctionDecl* func) {
    std::stringstream ss;
    ss << "\n{\n";
    ss << "\"qualified_name\": " << JsonEscape(func->getQualifiedNameAsString()) << ",\n";
    ss << "\"name\": " << JsonEscape(func->getNameAsString()) << ",\n";
    ss << "\"params\": [";
    bool step = false;
    for (const auto& param : func->params()) {
        if (step) ss << ",";
        ss << JsonEscape(param->getType().getAsString());
        step = true;
    }
    ss << "],\n";
    ss << "\"return\": " << JsonEscape(func->getReturnType().getAsString()) << "\n";
    ss << "}";
    return ss.str();
}

std::vector<std::string> opnamespaces;
std::vector<std::string> opclasses;
std::vector<std::string> opfunctions;

std::set<const FileEntry*> input_files;

class RecordPrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto Item = Result.Nodes.getDeclAs<CXXRecordDecl>("recordMatch");
        if (Item) {
            auto entry = Result.SourceManager->getFileEntryForID(Result.SourceManager->getFileID(Item->getLocation()));
            if (input_files.find(entry) != input_files.end()) {
                opclasses.push_back(DumpToJson(Item));
            }
        }
    }
};

class FunctionPrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        Result.Context->ExternalSource;
        auto Item = Result.Nodes.getDeclAs<FunctionDecl>("functionMatch");
        if (Item) {
            auto entry = Result.SourceManager->getFileEntryForID(Result.SourceManager->getFileID(Item->getLocation()));
            if (input_files.find(entry) != input_files.end()) {
                opfunctions.push_back(DumpToJson(Item));
            }
        }
    }
};

class NamespacePrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto Item = Result.Nodes.getDeclAs<NamespaceDecl>("namespaceMatch");
        if (Item && !Item->isAnonymousNamespace() 
            && std::find(opnamespaces.begin(), opnamespaces.end(), Item->getQualifiedNameAsString()) == opnamespaces.end()) {

            auto entry = Result.SourceManager->getFileEntryForID(Result.SourceManager->getFileID(Item->getLocation()));
            if (input_files.find(entry) != input_files.end()) {
                opnamespaces.push_back(Item->getQualifiedNameAsString());
            }
        }
    }
};

int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  auto source_list = OptionsParser.getSourcePathList();
  ClangTool Tool(OptionsParser.getCompilations(), source_list);
  auto& fm = Tool.getFiles();
  for (const auto& f : source_list) {
      input_files.insert(fm.getFile(f));
  }

  RecordPrinter RePrinter;
  FunctionPrinter FunPrinter;
  NamespacePrinter NsPrinter;
  MatchFinder Finder;
  Finder.addMatcher(recordDecl(isDefinition()).bind("recordMatch"), &RePrinter);
  Finder.addMatcher(functionDecl().bind("functionMatch"), &FunPrinter);
  Finder.addMatcher(namespaceDecl().bind("namespaceMatch"), &NsPrinter);

  int result = Tool.run(newFrontendActionFactory(&Finder).get());
  if (result == 0) {
      std::cout << "{\n\"namespaces\": [";
      for (size_t i = 0; i < opnamespaces.size(); ++i) {
          if (i > 0)
              std::cout << ",";
          std::cout << JsonEscape(opnamespaces[i]);
      }

      std::cout << "\n],\n\"classes\": [";
      for (size_t i = 0; i < opclasses.size(); ++i) {
          if (i > 0)
              std::cout << ",";
          std::cout << opclasses[i];
      }
      std::cout << "\n],\n\"functions\": [";
      for (size_t i = 0; i < opfunctions.size(); ++i) {
          if (i > 0)
              std::cout << ",";
          std::cout << opfunctions[i];
      }
      std::cout << "\n]\n}\n";
  }
  return result;
}
