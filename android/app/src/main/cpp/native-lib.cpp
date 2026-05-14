#include <jni.h>
#include <string>
#include <vector>
#include <set>

#include "mkpass.h"
#include "context.h"
#include "db.h"
#include "character_classes.h"
#include "qrcodegen.hpp"
#include "passphrase_patterns.h"

// Helper to convert jstring to std::string
std::string jstringToString(JNIEnv* env, jstring jstr) {
    if (jstr == nullptr) {
        return "";
    }
    const char* cstr = env->GetStringUTFChars(jstr, nullptr);
    std::string str(cstr);
    env->ReleaseStringUTFChars(jstr, cstr);
    return str;
}

// Helper to convert std::string to jstring
jstring stringToJstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

using qrcodegen::QrCode;
using qrcodegen::QrSegment;

extern "C" JNIEXPORT jobject JNICALL
Java_app_mkpass_MainActivity_generateQrCode(JNIEnv *env, jobject /* this */, jstring text) {
    const char *text_cstr = env->GetStringUTFChars(text, nullptr);
    const QrCode qr = QrCode::encodeText(text_cstr, QrCode::Ecc::MEDIUM);
    env->ReleaseStringUTFChars(text, text_cstr);

    const int size = qr.getSize();
    const int border = 4;
    const int bitmap_size = (size + border * 2) * 20;

    jclass bitmapCls = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapMid = env->GetStaticMethodID(bitmapCls, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    jstring configName = env->NewStringUTF("ARGB_8888");
    jclass configCls = env->FindClass("android/graphics/Bitmap$Config");
    jmethodID valueOfMid = env->GetStaticMethodID(configCls, "valueOf", "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;");
    jobject bitmapConfig = env->CallStaticObjectMethod(configCls, valueOfMid, configName);
    jobject bitmap = env->CallStaticObjectMethod(bitmapCls, createBitmapMid, bitmap_size, bitmap_size, bitmapConfig);

    jclass canvasCls = env->FindClass("android/graphics/Canvas");
    jmethodID canvasCtor = env->GetMethodID(canvasCls, "<init>", "(Landroid/graphics/Bitmap;)V");
    jobject canvas = env->NewObject(canvasCls, canvasCtor, bitmap);

    jclass paintCls = env->FindClass("android/graphics/Paint");
    jmethodID paintCtor = env->GetMethodID(paintCls, "<init>", "()V");
    jobject paint = env->NewObject(paintCls, paintCtor);

    jmethodID setColorMid = env->GetMethodID(paintCls, "setColor", "(I)V");
    jmethodID drawRectMid = env->GetMethodID(canvasCls, "drawRect", "(FFFFLandroid/graphics/Paint;)V");

    // Fill background with white
    env->CallVoidMethod(paint, setColorMid, 0xFFFFFFFF);
    env->CallVoidMethod(canvas, drawRectMid, 0.0f, 0.0f, (float)bitmap_size, (float)bitmap_size, paint);

    // Draw QR code modules
    env->CallVoidMethod(paint, setColorMid, 0xFF000000);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qr.getModule(x, y)) {
                float left = (x + border) * 20;
                float top = (y + border) * 20;
                env->CallVoidMethod(canvas, drawRectMid, left, top, left + 20, top + 20, paint);
            }
        }
    }

    return bitmap;
}


std::unique_ptr<mkpass::ConfigDB> db;

extern "C" JNIEXPORT void JNICALL
Java_app_mkpass_MainActivity_init(JNIEnv *env, jobject /* this */, jstring dbPath) {
    db = std::make_unique<mkpass::ConfigDB>(jstringToString(env, dbPath));
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_app_mkpass_MainActivity_getAllServiceNames(JNIEnv *env, jobject /* this */) {
    std::set<std::string> service_names = db->get_all_service_names();

    jobjectArray result = env->NewObjectArray(service_names.size(), env->FindClass("java/lang/String"), nullptr);
    int i = 0;
    for (const auto& s : service_names) {
        env->SetObjectArrayElement(result, i++, stringToJstring(env, s));
    }
    return result;
}

extern "C" JNIEXPORT jobject JNICALL
Java_app_mkpass_MainActivity_getServiceEntry(JNIEnv *env, jobject /* this */, jstring serviceName) {
    auto entry = db->get_service_entry(jstringToString(env, serviceName));

    if (!entry) {
        return nullptr;
    }

    jclass serviceEntryClass = env->FindClass("app/mkpass/ServiceEntry");
    jmethodID constructor = env->GetMethodID(serviceEntryClass, "<init>", "()V");
    jobject result = env->NewObject(serviceEntryClass, constructor);

    jfieldID algorithmField = env->GetFieldID(serviceEntryClass, "algorithm", "I");
    jfieldID lengthField = env->GetFieldID(serviceEntryClass, "length", "I");
    jfieldID charClassesField = env->GetFieldID(serviceEntryClass, "charClasses", "[I");
    jfieldID customCharsField = env->GetFieldID(serviceEntryClass, "customChars", "Ljava/lang/String;");
    jfieldID separatorField = env->GetFieldID(serviceEntryClass, "separator", "Ljava/lang/String;");
    jfieldID capitalizeWordsField = env->GetFieldID(serviceEntryClass, "capitalizeWords", "Z");
    jfieldID patternField = env->GetFieldID(serviceEntryClass, "pattern", "Ljava/lang/String;");
    jfieldID allowSubstitutionsField = env->GetFieldID(serviceEntryClass, "allowSubstitutions", "Z");

    env->SetIntField(result, algorithmField, static_cast<int>(entry->algorithm));
    env->SetIntField(result, lengthField, entry->length);
    env->SetObjectField(result, separatorField, stringToJstring(env, entry->separator));
    env->SetBooleanField(result, capitalizeWordsField, entry->capitalize_words);
    env->SetObjectField(result, patternField, stringToJstring(env, mkpass::PatternToString(entry->passphrase_pattern)));
    env->SetBooleanField(result, allowSubstitutionsField, entry->allow_substitutions);
    jintArray charClassesArray = env->NewIntArray(entry->char_classes.size());
    std::vector<jint> temp;
    for(const auto& cc : entry->char_classes) {
        temp.push_back(static_cast<int>(cc));
    }
    env->SetIntArrayRegion(charClassesArray, 0, temp.size(), temp.data());
    env->SetObjectField(result, charClassesField, charClassesArray);

    if (entry->custom_chars) {
        env->SetObjectField(result, customCharsField, stringToJstring(env, *entry->custom_chars));
    }

    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_app_mkpass_MainActivity_saveServiceEntry(JNIEnv *env, jobject /* this */, jstring serviceName, jint algorithm, jint length, jintArray charClasses, jstring customChars, jstring separator, jboolean capitalizeWords, jstring pattern, jboolean allowSubstitutions) {
    std::vector<CharacterClass> cc_vec;
    jint* cc_arr = env->GetIntArrayElements(charClasses, nullptr);
    int len = env->GetArrayLength(charClasses);
    for (int i=0; i<len; ++i) {
        cc_vec.push_back(static_cast<CharacterClass>(cc_arr[i]));
    }
    env->ReleaseIntArrayElements(charClasses, cc_arr, 0);

    std::optional<std::string> custom_chars_opt;
    if (customChars != nullptr) {
        custom_chars_opt = jstringToString(env, customChars);
    }

    db->save_service_entry({
        jstringToString(env, serviceName),
        static_cast<Algorithm>(algorithm),
        static_cast<unsigned>(length),
        cc_vec,
        custom_chars_opt,
        jstringToString(env, separator),
        mkpass::StringToPattern(jstringToString(env, pattern)),
        static_cast<bool>(allowSubstitutions),
        static_cast<bool>(capitalizeWords)
    });
}

extern "C" JNIEXPORT void JNICALL
Java_app_mkpass_MainActivity_deleteServiceEntry(JNIEnv *env, jobject /* this */, jstring serviceName) {
    db->delete_service_entry(jstringToString(env, serviceName));
}

extern "C" JNIEXPORT jstring JNICALL
Java_app_mkpass_MainActivity_generatePasswordNative(JNIEnv *env, jobject /* this */, jstring password, jstring service, jint algorithm, jint length, jintArray charClasses, jstring customChars, jstring separator, jboolean capitalizeWords, jstring pattern, jboolean allowSubstitutions) {
    std::vector<CharacterClass> cc_vec;
    jint* cc_arr = env->GetIntArrayElements(charClasses, nullptr);
    int len = env->GetArrayLength(charClasses);
    for (int i=0; i<len; ++i) {
        cc_vec.push_back(static_cast<CharacterClass>(cc_arr[i]));
    }
    env->ReleaseIntArrayElements(charClasses, cc_arr, 0);

    std::optional<std::string> custom_chars_opt;
    if (customChars != nullptr) {
        custom_chars_opt = jstringToString(env, customChars);
    }

    Context ctx = {
        .password = jstringToString(env, password),
        .service = jstringToString(env, service),
        .char_classes = cc_vec,
        .algorithm = static_cast<Algorithm>(algorithm),
        .separator = jstringToString(env, separator),
        .length = static_cast<unsigned>(length),
        .custom_chars = custom_chars_opt,
        .passphrase_pattern = mkpass::StringToPattern(jstringToString(env, pattern)),
        .allow_substitutions = static_cast<bool>(allowSubstitutions),
        .capitalize_words = static_cast<bool>(capitalizeWords)
    };


    std::string result = MkPass(ctx);
    return stringToJstring(env, result);
}

extern "C" JNIEXPORT jint JNICALL
Java_app_mkpass_MainActivity_getMaxPassphrasePatternLengthNative(JNIEnv *env, jobject /* this */) {
    return GetMaxPassphrasePatternLength();
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_app_mkpass_MainActivity_getPassphrasePatternsNative(JNIEnv *env, jobject /* this */, jint length) {
    PatternsList patterns = GetPassphrasePatterns(length);
    jobjectArray result = env->NewObjectArray(patterns.size(), env->FindClass("java/lang/String"), nullptr);
    for (size_t i = 0; i < patterns.size(); ++i) {
        env->SetObjectArrayElement(result, i, stringToJstring(env, mkpass::PatternToString(patterns[i])));
    }
    return result;
}
