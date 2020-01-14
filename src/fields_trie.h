#ifndef FIELDS_TRIE_H
#define FIELDS_TRIE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fields_trie 
{
    int states;
    short int ** trie;
} fields_trie;

/* */
void fields_trie_init(fields_trie* ftrie);

/* */
int fields_trie_find(fields_trie* ftrie, const char* input, int len);

#ifdef __cplusplus
}
#endif
#endif
