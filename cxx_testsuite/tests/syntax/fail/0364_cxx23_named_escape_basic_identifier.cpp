// RUN: -std=c++23
// EXPECT: Universal character name cannot name a basic character

int \N{LATIN CAPITAL LETTER A} = 1;
