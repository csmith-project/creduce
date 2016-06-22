//===----------------------------------------------------------------------===//
//
// Copyright (c) 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REPLACE_ARRAY_ACCESS_WITH_INDEX
#define REPLACE_ARRAY_ACCESS_WITH_INDEX

#include "Transformation.h"

class ReplaceArrayAccessWithIndex : public Transformation {
public:

  ReplaceArrayAccessWithIndex(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc)
    , Collector(NULL)
  { }

  ~ReplaceArrayAccessWithIndex(void);

private:
  class IndexCollector;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void doRewrite(void);

  IndexCollector *Collector;

  std::vector<clang::ArraySubscriptExpr const*> ASEs;

  // Unimplemented
  ReplaceArrayAccessWithIndex(void);
  ReplaceArrayAccessWithIndex(const ReplaceArrayAccessWithIndex &);
  void operator=(const ReplaceArrayAccessWithIndex &);
};

#endif

/*
  Local Variables:
  mode: c++
  fill-column: 79
  End:
*/
