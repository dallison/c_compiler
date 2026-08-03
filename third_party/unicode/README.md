# Unicode Character Database

DaveCC pins Unicode 17.0.0 for C++23 named universal character escapes. The
files in `17.0.0` come from:

- `https://www.unicode.org/Public/17.0.0/ucd/UnicodeData.txt`
- `https://www.unicode.org/Public/17.0.0/ucd/NameAliases.txt`

Their SHA-256 digests are:

```text
2e1efc1dcb59c575eedf5ccae60f95229f706ee6d031835247d843c11d96470c  UnicodeData.txt
793f6f1e4d15fd90f05ae66460191dc4d75d1fea90136a25f30dd6a4cb950eac  NameAliases.txt
```

Regenerate the checked-in lookup with:

```sh
python3 tools/generate_unicode_name_lookup.py \
  --ucd third_party/unicode/17.0.0 \
  --output c_compiler/frontend/lexical/generated \
  --version 17.0.0
```

The Unicode data files are distributed under the terms in
`17.0.0/LICENSE`.
