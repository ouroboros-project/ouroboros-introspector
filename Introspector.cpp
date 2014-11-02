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

std::string ToString(AccessSpecifier access) {
    switch (access) {
        case AccessSpecifier::AS_private: return "private";
        case AccessSpecifier::AS_protected: return "protected";
        case AccessSpecifier::AS_public: return "public";
        default: return "public";
    }
}

std::string ToString(QualType type) {
    std::string str = type.getAsString();
    if (str == "_Bool")
        return "bool";
    return str;
}

//
static void DumpToJsonCommon(std::ostream& ss, const NamedDecl* decl) {
    ss << " \n\"qualified_name\": " << JsonEscape(decl->getQualifiedNameAsString());
    ss << ",\n\"name\": " << JsonEscape(decl->getNameAsString());
    ss << ",\n\"access\": " << JsonEscape(ToString(decl->getAccess()));
}

static std::string DumpToJsonCommonFunc(const FunctionDecl* func) {
    std::stringstream ss;
    ss << "\n{";
    DumpToJsonCommon(ss, func);
    
    //
    ss << ",\n\"params\": [";
    bool step = false;
    for (const auto& param : func->params()) {
        if (step) ss << ",";
        ss << JsonEscape(ToString(param->getType()));
        step = true;
    }
    ss << "]";
    //

    if (!isa<CXXConstructorDecl>(func) && !isa<CXXDestructorDecl>(func))
        ss << ",\n\"return\": " << JsonEscape(ToString(func->getReturnType()));
    ss << ",\n\"deleted\": " << JsonEscape(func->isDeleted());
    return ss.str();
}
static std::string DumpToJson(const FunctionDecl* func) {
    return DumpToJsonCommonFunc(func) + "}";
}

static std::string DumpToJson(const CXXMethodDecl* method) {
    std::stringstream ss;
    ss << DumpToJsonCommonFunc(method);
    ss << ",\n\"virtual\": " << JsonEscape(method->isVirtual());
    ss << ",\n\"pure\": " << JsonEscape(method->isPure());
    ss << ",\n\"const\": " << JsonEscape(method->isConst());
    ss << ",\n\"static\": " << JsonEscape(method->isStatic());
    ss << "}";
    return ss.str();
}

static std::string DumpToJson(const CXXRecordDecl* cl) {
    std::stringstream ss;
    ss << "\n{";
    DumpToJsonCommon(ss, cl);

    ss << ",\n\"methods\": [";
    {
        bool step = false;
        for (const auto& method : cl->methods()) {
            if (method->isImplicit()) continue;
            if (step) ss << ",";
            ss << DumpToJson(method);
            step = true;
        }
    }
    ss << "]";

    ss << ",\n\"base_class\": [";
    {
        bool step = false;
        for (const auto& base: cl->bases()) {
            if (step) ss << ",";
            ss << "{ \"access\": " << JsonEscape(ToString(base.getAccessSpecifier()));
            ss << ", \"name\": " << JsonEscape(base.getType()->getAsCXXRecordDecl()->getQualifiedNameAsString()) << " }";
            step = true;
        }
    }
    ss << "]";

    ss << "}";
    return ss.str();
}

static std::string DumpToJson(const EnumDecl* en) {
    std::stringstream ss;
    ss << "\n{";
    DumpToJsonCommon(ss, en);
    ss << ",\n\"scoped\": " << JsonEscape(en->isScoped());

    ss << ",\n\"values\": [";
    {
        bool step = false;
        for (const auto& enumeration : en->enumerators()) {
            if (step) ss << ",";
            ss << JsonEscape(enumeration->getNameAsString());
            step = true;
        }
    }
    ss << "]";

    ss << "}";
    return ss.str();
}

static std::string DumpToJson(const VarDecl* var) {
    std::stringstream ss;
    ss << "\n{";
    DumpToJsonCommon(ss, var);
    ss << ",\n\"type\": " << JsonEscape(ToString(var->getType()));
    ss << ",\n\"static\": " << JsonEscape(var->isStaticDataMember());
    ss << "}";
    return ss.str();
}

static std::string DumpToJson(const FieldDecl* var) {
    std::stringstream ss;
    ss << "\n{";
    DumpToJsonCommon(ss, var);
    ss << ",\n\"type\": " << JsonEscape(ToString(var->getType()));
    ss << ",\n\"static\": " << JsonEscape("false");
    ss << "}";
    return ss.str();
}

std::vector<std::string> opnamespaces;
std::vector<std::string> opclasses;
std::vector<std::string> openums;
std::vector<std::string> opfunctions;
std::vector<std::string> opvariables;

std::set<const FileEntry*> input_files;

class RecordPrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto Item = Result.Nodes.getDeclAs<CXXRecordDecl>("recordMatch");
        if (Item) {
            auto entry = Result.SourceManager->getFileEntryForID(Result.SourceManager->getFileID(Item->getLocation()));
            if (input_files.find(entry) != input_files.end()) {
                auto x = Item->getDescribedClassTemplate();
                auto name = Item->getNameAsString();
                if (!x && !isa<ClassTemplateSpecializationDecl>(Item))
                    opclasses.push_back(DumpToJson(Item));
            }
        }
    }
};

class EnumPrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto Item = Result.Nodes.getDeclAs<EnumDecl>("enumMatch");
        if (Item && !Item->isTemplateDecl()) {
            auto entry = Result.SourceManager->getFileEntryForID(Result.SourceManager->getFileID(Item->getLocation()));
            if (input_files.find(entry) != input_files.end()) {
                openums.push_back(DumpToJson(Item));
            }
        }
    }
};

class FunctionPrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        Result.Context->ExternalSource;
        auto Item = Result.Nodes.getDeclAs<FunctionDecl>("functionMatch");
        if (Item && !Item->isCXXInstanceMember() && !Item->isTemplateDecl()) {
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

class VariablePrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto Item = Result.Nodes.getDeclAs<VarDecl>("variableMatch");
        if (Item && !Item->isTemplateDecl()) {
            auto entry = Result.SourceManager->getFileEntryForID(Result.SourceManager->getFileID(Item->getLocation()));
            if (input_files.find(entry) != input_files.end()) {
                if (!isa<ParmVarDecl>(Item))
                    opvariables.push_back(DumpToJson(Item));
            }
        }
    }
};

class FieldPrinter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) {
        auto Item = Result.Nodes.getDeclAs<FieldDecl>("fieldMatch");
        if (Item && !Item->isTemplateDecl()) {
            auto entry = Result.SourceManager->getFileEntryForID(Result.SourceManager->getFileID(Item->getLocation()));
            if (input_files.find(entry) != input_files.end()) {
                opvariables.push_back(DumpToJson(Item));
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
  EnumPrinter EnPrinter;
  FunctionPrinter FunPrinter;
  NamespacePrinter NsPrinter;
  VariablePrinter VarPrinter;
  FieldPrinter FieldPrinter;

  MatchFinder Finder;
  Finder.addMatcher(recordDecl(isDefinition()).bind("recordMatch"), &RePrinter);
  Finder.addMatcher(enumDecl().bind("enumMatch"), &EnPrinter);
  Finder.addMatcher(functionDecl().bind("functionMatch"), &FunPrinter);
  Finder.addMatcher(namespaceDecl().bind("namespaceMatch"), &NsPrinter);
  Finder.addMatcher(varDecl().bind("variableMatch"), &VarPrinter);
  Finder.addMatcher(fieldDecl().bind("fieldMatch"), &FieldPrinter);

  int result = Tool.run(newFrontendActionFactory(&Finder).get());
  if (result == 0) {
      std::cout << "{\n\"headers\": [";
      for (size_t i = 0; i < source_list.size(); ++i) {
          if (i > 0)
              std::cout << ",";
          std::cout << JsonEscape(source_list[i]);
      }

      std::cout << "\n],\n\"namespaces\": [";
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

      std::cout << "\n],\n\"enums\": [";
      for (size_t i = 0; i < openums.size(); ++i) {
          if (i > 0)
              std::cout << ",";
          std::cout << openums[i];
      }

      std::cout << "\n],\n\"functions\": [";
      for (size_t i = 0; i < opfunctions.size(); ++i) {
          if (i > 0)
              std::cout << ",";
          std::cout << opfunctions[i];
      }
      std::cout << "\n],\n\"variables\": [";
      for (size_t i = 0; i < opvariables.size(); ++i) {
          if (i > 0)
              std::cout << ",";
          std::cout << opvariables[i];
      }
      std::cout << "\n]\n}\n";
  }
  return result;
}
