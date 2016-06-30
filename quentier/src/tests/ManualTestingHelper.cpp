#include "ManualTestingHelper.h"
#include <QFile>

namespace quentier {
namespace test {

QString ManualTestingHelper::noteContentWithEncryption()
{
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
           "<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">"
           "<en-note>"
           "<h3>This note contains encrypted text</h3>"
           "<br/>"
           "<div>Here's the encrypted text containing only the hint attribute</div>"
           "<en-crypt hint=\"this is my rifle, this is my gun\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
           "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
           "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
           "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
           "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
           "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
           "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
           "<br/><div>Here's the encrypted text containing only the cipher attribute</div>"
           "<en-crypt cipher=\"AES\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
           "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
           "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
           "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
           "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
           "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
           "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
           "<br/><div>Here's the encrypted text containing only the length attribute</div>"
           "<en-crypt length=\"128\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
           "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
           "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
           "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
           "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
           "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
           "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
           "<br/><div>Here's the encrypted text containing cipher and length attributes</div>"
           "<en-crypt cipher=\"AES\" length=\"128\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
           "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
           "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
           "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
           "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
           "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
           "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
           "<br/><div>Here's the encrypted text containing cipher and hint attributes</div>"
           "<en-crypt hint=\"this is my rifle, this is my gun\" cipher=\"AES\">"
           "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
           "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
           "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
           "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
           "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
           "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
           "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
           "<br/><div>Here's the encrypted text containing length and hint attributes</div>"
           "<en-crypt hint=\"this is my rifle, this is my gun\" length=\"128\">"
           "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
           "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
           "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
           "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
           "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
           "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
           "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
           "<br/><div>Here's the encrypted text containing cipher, length and hint attributes</div>"
           "<en-crypt hint=\"this is my rifle, this is my gun\" cipher=\"AES\" length=\"128\">"
           "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
           "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
           "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
           "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
           "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
           "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
           "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
           "<div>Here's the text encrypted with RC2 which should reside in decrypted text cache</div>"
           "<en-crypt hint=\"my_own_encryption_key_1988\">"
           "K+sUXSxI2Mt075+pSDxR/gnCNIEnk5XH1P/D0Eie17"
           "JIWgGnNo5QeMo3L0OeBORARGvVtBlmJx6vJY2Ij/2En"
           "MVy6/aifSdZXAxRlfnTLvI1IpVgHpTMzEfy6zBVMo+V"
           "Bt2KglA+7L0iSjA0hs3GEHI6ZgzhGfGj</en-crypt>"
           "<div>Here's the text encrypted with AES which should reside in decrypted text cache</div>"
           "<en-crypt hint=\"MyEncryptionPassword\">"
           "RU5DMBwXjfKR+x9ksjSJhtiF+CxfwXn2Hf/WqdVwLwJDX9YX5R34Z5SBMSCIOFF"
           "r1MUeNkzHGVP5fHEppUlIExDG/Vpjh9KK1uu0VqTFoUWA0IXAAMA5eHnbxhBrjvL"
           "3CoTQV7prRqJVLpUX77Q0vbNims1quxVWaf7+uVeK60YoiJnSOHvEYptoOs1FVfZ"
           "AwnDDBoCUOsAb2nCh2UZ6LSFneb58xQ/6WeoQ7QDDHLSoUIXn</en-crypt>"
           "</en-note>";
}

QString ManualTestingHelper::noteContentWithResources()
{
    QFile file(":/test_notes/enml_with_resources.xml");
    file.open(QIODevice::ReadOnly);
    QString result = file.readAll();
    file.close();

    return result;
}

} // namespace test
} // namespace quentier
