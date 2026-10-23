/* Host execution of the production queue. Only ART entry points are fake. */
#include "../text_input_jni.c"
#include <assert.h>
#include <stdarg.h>
#ifdef NDEBUG
#error Assertions required
#endif

int g_swkbd_name_panel, g_swkbd_numeric_max;
_Atomic int g_fe_name_no_accept;
size_t swkbd_u8_to_u16(const char *s, size_t max, uint16_t *out, size_t cap) {
    size_t n = strlen(s); if (n > max) n = max; if (n > cap) n = cap;
    for (size_t i = 0; i < n; ++i) out[i] = (unsigned char)s[i]; return n;
}
size_t swkbd_u16_to_u8(const uint16_t *s, size_t n, char *out, size_t cap) {
    if (n >= cap) n = cap - 1; for (size_t i = 0; i < n; ++i) out[i] = (char)s[i]; out[n] = 0; return n;
}
int dm_name_taken(const char *n) { (void)n; return 0; }
int dm_name_gen(char *o, int c, int (*taken)(const char *)) { (void)o; (void)c; (void)taken; return 0; }
int __android_log_print(int p, const char *tag, const char *fmt, ...) { (void)p; (void)tag; (void)fmt; return 0; }
const char *dm_android_diagnostics_runtime(void) { return "/private/dm"; }
void dm_android_diagnostics_snapshot(char *out, size_t cap, uint64_t now) { assert(now); snprintf(out, cap, "native_metadata=yes\n"); }

static pthread_t request_thread;
static pthread_mutex_t test_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t test_done = PTHREAD_COND_INITIALIZER;
static int calls[5], attached;
static JNIEnv test_env;
static jclass find(JNIEnv *e, const char *name) { (void)e; assert(!strcmp(name, "com/xiaoboluo/d2r/DmActivity")); return (jclass)(uintptr_t)1; }
static jobject global(JNIEnv *e, jobject v) { (void)e; return v; }
static void release(JNIEnv *e, jobject v) { (void)e; if ((uintptr_t)v != 1) free(v); }
static jmethodID method(JNIEnv *e, jclass c, const char *n, const char *sig) {
    (void)e; (void)c; (void)sig;
    const char *names[] = {"", "showNativeInput", "showNativeFailure", "showNativeDriverSettings", "showNativeDiagnostics"};
    for (unsigned i = 1; i < 5; ++i) if (!strcmp(n, names[i])) return (jmethodID)(uintptr_t)i;
    return NULL;
}
static jboolean exception(JNIEnv *e) { (void)e; return 0; }
static void clear_exception(JNIEnv *e) { (void)e; }
static jstring new_string(JNIEnv *e, const jchar *chars, jsize n) {
    (void)e; jchar *copy = calloc(n + 1, sizeof *copy); assert(copy); memcpy(copy, chars, n * sizeof *copy); return (jstring)copy;
}
static const jchar *get_chars(JNIEnv *e, jstring s, jboolean *copy) { (void)e; (void)copy; return (jchar *)s; }
static jsize get_length(JNIEnv *e, jstring s) { (void)e; jchar *p = (jchar *)s; jsize n = 0; while (p[n]) ++n; return n; }
static void release_chars(JNIEnv *e, jstring s, const jchar *p) { (void)e; (void)s; (void)p; }
static void call(JNIEnv *e, jclass c, jmethodID method_id, ...) {
    (void)c; assert(!pthread_equal(pthread_self(), request_thread));
    int id = (int)(uintptr_t)method_id; va_list args; va_start(args, method_id);
    if (id == 4) {
        jstring runtime = va_arg(args, jstring), metadata = va_arg(args, jstring);
        assert(((jchar *)runtime)[0] == '/'); assert(((jchar *)metadata)[0] == 'n');
    } else if (id == 1) {
        jlong request = va_arg(args, jlong); const jchar accepted[] = {'H','e','r','o',0};
        Java_com_xiaoboluo_d2r_DmActivity_nativeTextResult(e, c, request, JNI_TRUE, (jstring)accepted);
    } else if (id == 2) assert(va_arg(args, int) == -7);
    va_end(args);
    pthread_mutex_lock(&test_lock); ++calls[id]; pthread_cond_signal(&test_done); pthread_mutex_unlock(&test_lock);
}
static jint get_env(JavaVM *v, void **env, jint version) { (void)v; assert(version == JNI_VERSION_1_6); *env = &test_env; return JNI_OK; }
static jint attach(JavaVM *v, JNIEnv **env, void *args) {
    (void)v; (void)args; assert(!pthread_equal(pthread_self(), request_thread));
    *env = &test_env; attached = 1; return JNI_OK;
}
int main(void) {
    static const struct JNINativeInterface table = {
        .FindClass=find, .NewGlobalRef=global, .DeleteLocalRef=release, .GetStaticMethodID=method,
        .ExceptionCheck=exception, .ExceptionClear=clear_exception, .ExceptionDescribe=clear_exception,
        .NewString=new_string, .CallStaticVoidMethod=call, .GetStringChars=get_chars,
        .GetStringLength=get_length, .ReleaseStringChars=release_chars
    };
    static const struct JNIInvokeInterface invoke = {.GetEnv=get_env, .AttachCurrentThreadAsDaemon=attach};
    test_env = &table; JavaVM fake_vm = &invoke; request_thread = pthread_self();
    assert(JNI_OnLoad(&fake_vm, NULL) == JNI_VERSION_1_6);
    dm_android_diagnostics_request(); dm_android_driver_settings_request(); front_android_boot_failed(-7);
    char output[40]; assert(front_android_text_input("Title", "Sub", "Guide", "", 20, output, sizeof output) == 1);
    assert(!strcmp(output, "Hero"));
    pthread_mutex_lock(&test_lock);
    struct timespec deadline; timespec_get(&deadline, TIME_UTC); deadline.tv_sec += 5;
    while (!calls[1] || !calls[2] || !calls[3] || !calls[4]) assert(!pthread_cond_timedwait(&test_done, &test_lock, &deadline));
    assert(attached && calls[1] == 1 && calls[2] == 1 && calls[3] == 1 && calls[4] == 1);
    pthread_mutex_unlock(&test_lock);
    puts("PASS actual native queue dispatch: original-stack worker, diagnostics, driver, failure and text result");
}
