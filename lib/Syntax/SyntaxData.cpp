//===--- SyntaxData.cpp - Swift Syntax Data Implementation ----------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "swift/Syntax/SyntaxData.h"

using namespace swift;
using namespace swift::syntax;

RC<SyntaxData> SyntaxData::make(RC<RawSyntax> Raw,
                                const SyntaxData *Parent,
                                CursorIndex IndexInParent) {
  auto size = totalSizeToAlloc<AtomicCache<SyntaxData>>(Raw->getNumChildren());
  void *data = ::operator new(size);
  return RC<SyntaxData>{new (data) SyntaxData(Raw, Parent, IndexInParent)};
}

bool SyntaxData::isType() const {
  return Raw->isType();
}

bool SyntaxData::isStmt() const {
  return Raw->isStmt();
}

bool SyntaxData::isDecl() const {
  return Raw->isDecl();
}

bool SyntaxData::isExpr() const {
  return Raw->isExpr();
}

bool SyntaxData::isPattern() const {
  return Raw->isPattern();
}

bool SyntaxData::isUnknown() const {
  return Raw->isUnknown();
}

void SyntaxData::dump(llvm::raw_ostream &OS) const {
  Raw->dump(OS, 0);
}

RC<SyntaxData> SyntaxData::getPreviousNode() const {
  if (size_t N = getIndexInParent()) {
    if (hasParent()) {
      for (size_t I = N - 1; ; I--) {
        if (auto C = getParent()->getChild(I)) {
          return C;
        }
        if (I == 0)
          break;
      }
    }
  }
  return hasParent() ? Parent->getPreviousNode() : nullptr;
}

RC<SyntaxData> SyntaxData::getNextNode() const {
  if (hasParent()) {
    for (size_t I = getIndexInParent() + 1, N = Parent->getNumChildren();
         I != N; I++) {
      if (auto C = getParent()->getChild(I))
        return C;
    }
    return Parent->getNextNode();
  }
  return nullptr;
}

AbsolutePosition SyntaxData::getAbsolutePositionWithLeadingTrivia() const {
  if (PositionCache.hasValue())
    return *PositionCache;
  if (auto P = getPreviousNode()) {
    auto Result = P->getAbsolutePositionWithLeadingTrivia();
    P->getRaw()->accumulateAbsolutePosition(Result);
    // FIXME: avoid using const_cast.
    const_cast<SyntaxData*>(this)->PositionCache = Result;
  } else {
    const_cast<SyntaxData*>(this)->PositionCache = AbsolutePosition();
  }
  return *PositionCache;
}

AbsolutePosition SyntaxData::getAbsolutePosition() const {
  auto Result = getAbsolutePositionWithLeadingTrivia();
  getRaw()->accumulateLeadingTrivia(Result);
  return Result;
}

AbsolutePosition SyntaxData::getAbsoluteEndPosition() const {
  if (auto N = getNextNode()) {
    return N->getAbsolutePositionWithLeadingTrivia();
  } else {
    auto Result = getAbsolutePositionWithLeadingTrivia();
    getRaw()->accumulateAbsolutePosition(Result);
    return Result;
  }
}
