#ifndef URL_TRIE_H
#define URL_TRIE_H

#ifdef __cplusplus
extern "C" {
#endif
typedef struct url_trie {
        int nstates;
        short **trie;
} url_trie;

/* */
void url_trie_init(url_trie *trie);

/* */
int url_trie_insert(url_trie *trie, const char *patt, int value);

/* */
int url_trie_find(url_trie *trie, const char *input, char *output[]);
#ifdef __cplusplus
}
#endif
#endif
