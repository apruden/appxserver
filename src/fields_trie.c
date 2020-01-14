#include <stdlib.h>
#include "fields_trie.h"

/* */
void fields_trie_init(fields_trie* ftrie)
{
    char *fields[36] = { "Accept", 
        "Accept-Charset",
        "Accept-Encoding",
        "Accept-Language",
        "Authorization",
        "Cache-Control",
        "Connection",
        "Cookie",
        "Content-Length",
        "Content-MD5",
        "Content-Type",
        "Date",
        "Expect",
        "From",
        "Host",
        "If-Match",
        "If-Modified-Since",
        "If-None-Match",
        "If-Range",
        "If-Unmodified-Since",
        "Max-Forwards",
        "Pragma",
        "Proxy-Authorization",
        "Range",
        "Referer",
        "TE",
        "Upgrade",
        "User-Agent",
        "Via",
        "Warning",
        "X-Requested-With",
        "X-Do-Not-Track",
        "DNT",
        "X-Forwarded-For",
        "X-ATT-DeviceId",
        "X-Wap-Profile"
    };

    int MAX_STATES = 500;
    ftrie->trie = (short **) malloc(MAX_STATES * sizeof(short *));
    int i, j;

    for(i = 0; i < MAX_STATES; i++)
    {
        ftrie->trie[i] = (short*) calloc(128, sizeof(short));
    }

    int nstates = 0;

    for(i = 0; i < 36; i++ )
    {
        nstates = fields_trie_insert(ftrie, fields[i], i);
    }

}

/* */
int fields_trie_insert(fields_trie* ftrie, const char* patt, int value)
{
    int i=0, j = 0;
    int nstates = ftrie->states;

    while(patt[j] != '\0')
    {
        if(ftrie->trie[i][patt[j]] == 0 )
        {
            nstates++;
            ftrie->trie[i][patt[j]] = nstates;
        }

        i = ftrie->trie[i][patt[j]];
        j++;
    }

    ftrie->trie[i][patt[j]] = value;
    ftrie->states = nstates;
    return nstates;
}

/* */
int fields_trie_find(fields_trie* ftrie, const char* input, int len)
{
    int i = 0, j = 0;

    for(j =0; j < len; j++)
    {
        if(ftrie->trie[i][input[j]] == 0 )
        {
            break;
        }

        i = ftrie->trie[i][input[j]];
    }

    return ftrie->trie[i][input[j]];
}
