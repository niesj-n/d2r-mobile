"""Synthetic map-goal staging tests, with no game text, sprites or fonts."""
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

SPEC=importlib.util.spec_from_file_location('stage_touch_assets',Path(__file__).resolve().parents[1]/'tools/stage_touch_assets.py')
stage=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(stage)
def png():
    def chunk(k,v):return struct.pack('>I',len(v))+k+v+struct.pack('>I',zlib.crc32(k+v)&0xffffffff)
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',1,1,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b'\x00\x00\x00\x00\x00'))+chunk(b'IEND',b'')
def fixture(root):
    root.mkdir(); rows=[]
    for i in range(31):
        row={'id':i,'zhCN':'synthetic','old':'old' if i==0 else 'synthetic'}
        rows.append(row)
        for suffix in ('','_en','_es','_mx','_ru','_ko')+(('_old',) if i==0 else ()):(root/f'label_{i}{suffix}.png').write_bytes(png())
    for name in ['cross.png']+[f'sym_{i}.png' for i in range(7)]:(root/name).write_bytes(png())
    (root/'mapgoal.json').write_text(json.dumps({'text_px':48,'symbol_frames':7,'labels':rows}),encoding='utf-8')
class GoalAssets(unittest.TestCase):
    def test_complete_and_missing(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)/'mapgoal';fixture(root)
            names=stage.map_goal_files(root)
            self.assertEqual(len(names),196);self.assertIn('label_0_old.png',names)
            (root/'sym_6.png').unlink()
            with self.assertRaisesRegex(ValueError,'map goal'):stage.map_goal_files(root)
    def test_corrupt_png_and_bad_ids(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)/'mapgoal';fixture(root)
            p=root/'cross.png';p.write_bytes(p.read_bytes()[:-8]+b'corrupt!')
            with self.assertRaisesRegex(ValueError,'map goal'):stage.map_goal_files(root)
            p.write_bytes(png());m=root/'mapgoal.json';v=json.loads(m.read_text());v['labels'][0]['id']='../outside';m.write_text(json.dumps(v))
            with self.assertRaisesRegex(ValueError,'map goal'):stage.map_goal_files(root)
    def test_extras_not_selected(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)/'mapgoal';fixture(root);(root/'font.ttf').write_bytes(b'private')
            self.assertNotIn('font.ttf',stage.map_goal_files(root))
if __name__=='__main__':unittest.main()
