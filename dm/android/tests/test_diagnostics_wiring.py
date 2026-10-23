"""Source boundary regression: an existing guest menu must enqueue, never enter ART."""
from pathlib import Path
import re
import unittest

ANDROID = Path(__file__).resolve().parents[1]

class DiagnosticsWiring(unittest.TestCase):
    def test_existing_menu_hook_registers_android_callback(self):
        front = (ANDROID / 'front_android.c').read_text(encoding='utf-8')
        diag = (ANDROID.parent / 'host/diag.c').read_text(encoding='utf-8')
        self.assertIn('dm_sharelog_init(dm_android_diagnostics_request)', front)
        self.assertIn('"TouchShareLog"', diag)
        self.assertIn('if (g_sharelog_cb) g_sharelog_cb()', diag)

    def test_guest_request_only_enqueues_and_original_stack_worker_dispatches(self):
        source = (ANDROID / 'text_input_jni.c').read_text(encoding='utf-8')
        request = re.search(r'void dm_android_diagnostics_request\(void\) \{(.*?)\n\}', source, re.S)
        self.assertIsNotNone(request)
        self.assertIn('enqueue(job)', request[1])
        self.assertNotRegex(request[1], r'CallStatic|AttachCurrent|JNIEnv')
        worker = source[source.index('static void *input_worker'):source.index('JNIEXPORT jint JNI_OnLoad')]
        self.assertIn('AttachCurrentThreadAsDaemon', worker)
        self.assertIn('activity_class, show_diagnostics, runtime, metadata', worker)
        for kept in ('show_driver_settings', 'report_failure', 'show_input', 'cancel_request'):
            self.assertIn(kept, worker)

    def test_android_saves_locally_off_ui_thread_without_network_permission(self):
        source = (ANDROID / 'app/src/main/java/com/xiaoboluo/d2r/DiagnosticsExporter.java').read_text(encoding='utf-8')
        self.assertIn('ACTION_CREATE_DOCUMENT', source)
        self.assertIn('"dm-diagnostics-save"', source)
        self.assertIn('DiagnosticsArchive.write(files, runtime, metadata, pipeline, stream)', source)
        self.assertLess(source.index('new Thread(() -> {'), source.index('nativePipelineDiagnostics();', source.index('void begin(')))
        self.assertNotIn('ACTION_SEND', source)
        manifest = (ANDROID / 'app/src/main/AndroidManifest.xml').read_text(encoding='utf-8')
        self.assertNotIn('android.permission.INTERNET', manifest)

if __name__ == '__main__': unittest.main()
