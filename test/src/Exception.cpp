#include "Exception.h"
#include <string.h>

using namespace penelope;

const unsigned int Exception::MSG_MAX_LENGTH = 512;

Exception::Exception(int anErrorCode, unsigned int anOrigin, const char* aMsg) : errorCode(anErrorCode),
origin(anOrigin), msg() {
    memset(msg, 0, MSG_MAX_LENGTH);
    strncpy(msg, aMsg, strnlen(aMsg, MSG_MAX_LENGTH - 1));
}

Exception::Exception(const Exception& other) : errorCode(other.errorCode), origin(other.origin), msg() {
    strncpy(msg, other.msg, MSG_MAX_LENGTH);
}

Exception& Exception::operator=(const Exception& other) {
    errorCode = other.errorCode;
    origin = other.origin;
    strncpy(msg, other.msg, MSG_MAX_LENGTH);
    return *this;
}
