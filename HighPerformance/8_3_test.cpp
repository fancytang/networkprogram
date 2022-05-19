#include<stdio.h>
#include<string.h>
#include<stdlib.h>

enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER };
enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };
enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, FORBIDDEN_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };

CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;

HTTP_CODE parse_requestline(char *temp, CHECK_STATE &checkstate) {
    char *url = strpbrk(temp, " \t");
    if (! url) {
        return BAD_REQUEST;
    }
    printf("%s",url);
    // *url++ = '\0';
    char *method = temp;
    if (strcasecmp(method, "GET") == 0) {
        printf("The request method is GET\n");
    } else {
        printf("only support GET method !\n");
        // return BAD_REQUEST;
    }
    printf("%s",method);
    // puts(method);

    // url += strspn(url, " \t");
    // puts(url);
    // char *version = strpbrk(url, " \t");
    // if (!version) {
    //     // return BAD_REQUEST;
    //     puts("ERROR");
    // }else{
    //     puts("OK");
    // }
    // puts(version);

    // // *version++ = '\0';
    // version += strspn(version, " \t");
    // if (strcasecmp(version, "HTTP/1.1") != 0) {
    //     // return BAD_REQUEST;
    //     puts("ERROR");
    // }
    // if (strncasecmp(url, "http://", 7) == 0) {
    //     url += 7;
    //     url = strchr(url, '/');
    // }
    // puts(url);

    // if (!url || url[0] != '/') {
    //     // return BAD_REQUEST;
    //     puts("ERROR");
    // }
    // printf("The request URL is : %s\n", url);
    // checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

int main(){
    char *buf = "GET /http://example/hello.html HTTP/1.1\r\n\r\nHost:hostlocal";
    HTTP_CODE code=parse_requestline(buf,checkstate);
    return 0;
}