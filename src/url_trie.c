#include <stdio.h>
#include <stdlib.h>
#include "url_trie.h"

/* */
void url_trie_init(url_trie *trie)
{
    int i = 0;
    int MAX_STATES = 500;

    trie->trie = (short**) malloc(MAX_STATES * sizeof(short*));

    for(i = 0 ; i < MAX_STATES; i++)
    {
        trie->trie[i] = (short*) calloc(128 , sizeof(short));
    }
}

/* */
int url_trie_insert(url_trie *urltrie, const char *patt, int value)
{
    int i=0, j = 0, k=0;
    short **trie = urltrie->trie;
    int nstates = urltrie->nstates;

    while(patt[j] != '\0')
    {
        if(trie[i][patt[j]] == 0 )
        {
            nstates++;
            trie[i][patt[j]] = nstates;

            if(patt[j] == '?')
            {
                k = 0;

                for(k = 0; k < 128; k++)
                {
                    trie[nstates][k] = nstates;
                }

                if( patt[j+1] == '\0')
                {
                    trie[nstates][0] = value;
                }

                trie[nstates]['/'] = nstates + 1;
                nstates++;
            }
        }

        i = trie[i][patt[j]];
        j++;
    }

    trie[i][patt[j]] = value;
    urltrie->nstates = nstates;

    return nstates;
}

/* */
int url_trie_find(url_trie *urltrie, const char *input, char *output[])
{
    int i = 0, j = 0, flag = 0, last  = 0, k = 0, l = 0;
    short **trie = urltrie->trie;

    while(input[j] != '\0')
    {
        if(trie[i][input[j]] == 0 )
        {
            if(flag)
            {
                output[k][l] = '\0';
                flag = 0;
                l = 0;
                k++;
            }

            if(trie[i]['?'] == 0) {
                break;
            }

            i = trie[i]['?'];
            output[k] = malloc(64 * sizeof(char));
            output[k][l] = input[j];
            l++;
            flag = 1;
            last = i;
            j++;
            continue;
        }

        i = trie[i][input[j]];

        if(flag){
            if (i == last)
            {
                output[k][l] = input[j];
                l++;
            }
            else
            {
                output[k][l] = '\0';
                flag = 0;
                l = 0;
                k++;
            }
        }

        j++;
    }

    return trie[i][input[j]];
}
