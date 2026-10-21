from pathlib import Path
import json, uuid
from datetime import datetime, timezone
ROOT=Path(__file__).resolve().parent.parent
STORE=ROOT/"data"/"enterprise"; STORE.mkdir(parents=True,exist_ok=True)
def now(): return datetime.now(timezone.utc).isoformat()
def _p(k): return STORE/f"{k}.jsonl"
def list_objects(k):
 p=_p(k)
 if not p.exists(): return []
 out=[]
 for line in p.read_text().splitlines():
  try: out.append(json.loads(line))
  except: pass
 return out
def get_object(k,i): return next((x for x in list_objects(k) if x.get("id")==i),None)
def upsert(k,o):
 rows=list_objects(k); o=dict(o)
 if not o.get("id"): o["id"]=f"{k[:3]}_{uuid.uuid4().hex[:12]}"; o["created_at"]=now()
 o["updated_at"]=now(); rows=[x for x in rows if x.get("id")!=o["id"]]+[o]
 _p(k).write_text("".join(json.dumps(x,separators=(",",":"))+"\n" for x in rows)); return o
