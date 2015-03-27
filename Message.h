#ifndef MESSAGE_H
#define MESSAGE_H

namespace Net
{

    struct Message
    {
        int type;
        void* data;
        int len;
    };
}

#endif
