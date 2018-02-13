/*!
 *  Copyright (c) 2015 by Contributors
 * \file export.cc
 * \brief Exporter module to export document and wrapper functions.
 */
#include <Rcpp.h>
#include <fstream>
#include <sstream>
#include "./base.h"
#include "./export.h"

namespace mxnet {
namespace R {
// docstring related function.
std::string MakeDocString(mx_uint num_args,
                          const char **arg_names,
                          const char **arg_type_infos,
                          const char **arg_descriptions,
                          bool remove_dup) {
  std::set<std::string> visited;
  std::ostringstream os;
  for (mx_uint i = 0; i < num_args; ++i) {
    std::string arg = arg_names[i];
    if (visited.count(arg) != 0 && remove_dup) continue;
    for (size_t j = 0; j < arg.length(); ++j) {
      if (arg[j] == '_') arg[j] = '.';
    }
    visited.insert(arg);
    os << "@param " << arg << "  "  << arg_type_infos[i] << "\n"
       << "    " << arg_descriptions[i] << "\n";
  }
  return os.str();
}


Exporter* Exporter::Get() {
  static Exporter inst;
  return &inst;
}

void Exporter::InitRcppModule() {
  using namespace Rcpp;  // NOLINT(*)
  Exporter::Get()->scope_ = ::getCurrentScope();
  function("mx.internal.export", &Exporter::Export,
           Rcpp::List::create(_["path"]),
           "Internal function of mxnet, used to export generated functions file.");
}

std::string ExportDocString(const std::string& docstring) {
  std::ostringstream os;
  std::istringstream is(docstring);
  std::string line;
  line.resize(1024);
  while (is.getline(&line[0], line.length())) {
    os << "#' " << line.c_str() << "\n";
  }
  return os.str();
}

void ExportVArgFunction(std::ostream& os,  // NOLINT(*)
                        const std::string& func_name,
                        const std::string& docstr) {
  std::string prefix = "mx.varg.";
  std::string new_name = std::string("mx.") + (func_name.c_str() + prefix.length());
  os << "\n" << ExportDocString(docstr)
     << new_name << " <- function(...) {\n"
     << "  " << func_name << "(list(...))\n"
     << "}\n";
  RLOG_INFO << "Exporting " << func_name << " as " << new_name << "\n";
}

void ExportNormalFunction(std::ostream& os,  // NOLINT(*)
                          const std::string& func_name,
                          const std::string& docstr) {
  os << "\n"
     << ExportDocString(docstr)
     << "#' @name " << func_name << "\n"
     << "NULL\n";
  RLOG_INFO << "Exporting " << func_name << " docstring\n";
}

void Exporter::Export(const std::string& path) {
  std::string filename = path + "/R/mxnet_generated.R";
  std::ofstream script(filename.c_str());
  RLOG_INFO << "Start to generate "<< path << " ...\n";
  script << "######\n"
         << "# Generated by mxnet.export, do not edit by hand.\n"
         << "######\n";
  Rcpp::Module *scope = Exporter::Get()->scope_;
  Rcpp::CharacterVector func_names = scope->functions_names();

  for (size_t i = 0; i < func_names.size(); ++i) {
    std::string fname = Rcpp::as<std::string>(func_names[i]);
    // skip internal functions
    if (fname.find("internal.") != std::string::npos) continue;
    if (fname == "mx.varg.symbol.Concat"
      || fname == "mx.varg.symbol.concat"
      || fname == "mx.varg.symbol.min_axis"
      || fname == "mx.varg.symbol.min") continue;
    Rcpp::List func_info(scope->get_function(fname));
    std::string docstr = Rcpp::as<std::string>(func_info[2]);
    if (docstr.find("@export") == std::string::npos) continue;
    if (fname.find("mx.varg.") == 0) {
      ExportVArgFunction(script, fname, docstr);
    } else {
      ExportNormalFunction(script, fname, docstr);
    }
  }
  RLOG_INFO << "All generation finished on "<< path << " ...\n";
}
}  // namespace R
}  // namespace mxnet