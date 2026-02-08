#include <jni.h>
#include <string>
#include <vector>
#include <set>

#include "mkpass.h"
#include "context.h"
#include "db.h"
#include "character_classes.h"

// Helper to convert std::string to jstring
jstring stringToJstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

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


extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_example_mkpass_MainActivity_getAllServiceNames(JNIEnv *env, jobject /* this */) {
    mkpass::ConfigDB db("mkpass.db"); // TODO: pass path as 3rd arg
    std::set<std::string> service_names = db.get_all_service_names();

    jobjectArray result = env->NewObjectArray(service_names.size(), env->FindClass("java/lang/String"), nullptr);
    int i = 0;
    for (const auto& s : service_names) {
        env->SetObjectArrayElement(result, i++, stringToJstring(env, s));
    }
    return result;
}

extern "C" JNIEXPORT jobject JNICALL
Java_com_example_mkpass_MainActivity_getServiceEntry(JNIEnv *env, jobject /* this */, jstring serviceName) {
    mkpass::ConfigDB db("mkpass.db");
    auto entry = db.get_service_entry(jstringToString(env, serviceName));

    if (!entry) {
        return nullptr;
    }

    jclass serviceEntryClass = env->FindClass("com/example/mkpass/ServiceEntry");
    jmethodID constructor = env->GetMethodID(serviceEntryClass, "<init>", "()V");
    jobject result = env->NewObject(serviceEntryClass, constructor);

    jfieldID algorithmField = env->GetFieldID(serviceEntryClass, "algorithm", "I");
    jfieldID lengthField = env->GetFieldID(serviceEntryClass, "length", "I");
    jfieldID charClassesField = env->GetFieldID(serviceEntryClass, "charClasses", "[I");
    jfieldID customCharsField = env->GetFieldID(serviceEntryClass, "customChars", "Ljava/lang/String;");

    env->SetIntField(result, algorithmField, static_cast<int>(entry->algorithm));
    env->SetIntField(result, lengthField, entry->length);

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
Java_com_example_mkpass_MainActivity_saveServiceEntry(JNIEnv *env, jobject /* this */, jstring serviceName, jint algorithm, jint length, jintArray charClasses, jstring customChars) {
    mkpass::ConfigDB db("mkpass.db");
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

    db.save_service_entry({
        jstringToString(env, serviceName),
        static_cast<Algorithm>(algorithm),
        static_cast<unsigned>(length),
        cc_vec,
        custom_chars_opt
    });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_mkpass_MainActivity_generatePasswordNative(JNIEnv *env, jobject /* this */, jstring password, jstring service, jint algorithm, jint length, jintArray charClasses, jstring customChars) {
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
        .is_gui = true,
        .length = static_cast<unsigned>(length),
        .custom_chars = custom_chars_opt
    };

    std::string result = MkPass(ctx);
    return stringToJstring(env, result);
}
