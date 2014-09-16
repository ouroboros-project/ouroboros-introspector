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

DeclarationMatcher NamespaceMatcher = namespaceDecl().bind("namespaceMatch");
DeclarationMatcher RecordMatcher = recordDecl(isDefinition()).bind("recordMatch");
DeclarationMatcher FunctionMatcher = functionDecl().bind("functionMatch");

class NamespacePrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto NS = Result.Nodes.getDeclAs<NamespaceDecl>("namespaceMatch");
        if (NS && NS->isOriginalNamespace() && !NS->isAnonymousNamespace()) {
            llvm::outs() << "Found namespace: " << NS->getName() << "\n";
        }
    }
};

class RecordPrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto Item = Result.Nodes.getDeclAs<CXXRecordDecl>("recordMatch");
        if (Item) {
            llvm::outs() << "Found class/struct/union: " << Item->getName() << "\n";
            for (const auto& method : Item->methods()) {
                llvm::outs() << "\tMethod: " << method->getName() << "\n";
            }
        }
    }
};

class FunctionPrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto Item = Result.Nodes.getDeclAs<FunctionDecl>("functionMatch");
        if (Item && !Item->isCXXInstanceMember()) {
            llvm::outs() << "Found function: " << Item->getName() << "\n";
        }
    }
};


//

int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  NamespacePrinter NSPrinter;
  RecordPrinter RePrinter;
  FunctionPrinter FunPrinter;
  MatchFinder Finder;
  Finder.addMatcher(NamespaceMatcher, &NSPrinter);
  Finder.addMatcher(RecordMatcher, &RePrinter);
  Finder.addMatcher(FunctionMatcher, &FunPrinter);

  return Tool.run(newFrontendActionFactory(&Finder).get());
}
