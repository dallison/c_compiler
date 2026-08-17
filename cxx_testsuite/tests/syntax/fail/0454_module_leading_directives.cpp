// RUN: -std=c++26
// EXPECT: only #line directives may precede the first module directive

#pragma once
export module invalid_leading_group;
