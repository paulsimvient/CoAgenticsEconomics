from pathlib import Path
p=Path('/mnt/data/v7build/workbench/interface.html')
s=p.read_text()
css=r'''
/* ===== CoAgentics Enterprise V7 — Workspace & Governance ===== */
.enterprise-v7{display:none}.workspace-menu,.governance-panel{position:fixed;z-index:150;background:var(--panel);border:1px solid var(--line);box-shadow:0 30px 90px rgba(0,0,0,.42);border-radius:12px}.workspace-menu{top:58px;left:245px;width:min(390px,calc(100vw - 30px));padding:10px}.workspace-head{padding:10px;border-bottom:1px solid var(--line);margin-bottom:7px}.workspace-head small,.governance-panel small{display:block;color:var(--muted);font:9px var(--mono);text-transform:uppercase;letter-spacing:.08em}.workspace-head b{display:block;font-size:13px;margin-top:3px}.workspace-option{width:100%;display:grid;grid-template-columns:28px 1fr auto;gap:9px;align-items:center;text-align:left;padding:9px;border:1px solid transparent;background:transparent;color:var(--text);border-radius:7px}.workspace-option:hover,.workspace-option.active{background:var(--accent-soft);border-color:var(--line)}.workspace-option .ws-icon{width:26px;height:26px;border:1px solid var(--accent);border-radius:6px;display:grid;place-items:center;color:var(--accent2);font:10px var(--mono)}.workspace-option b{font-size:11px}.workspace-option span{display:block;color:var(--muted);font:9px var(--mono);margin-top:2px}.workspace-option em{font-style:normal;font:8px var(--mono);color:var(--ok)}
.governance-panel{top:58px;right:18px;width:min(650px,calc(100vw - 30px));max-height:calc(100vh - 80px);overflow:auto;padding:16px}.gov-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:12px}.gov-card{border:1px solid var(--line);background:var(--panel2);border-radius:8px;padding:11px}.gov-card h3{font-family:var(--display);font-size:15px;margin:0 0 7px}.gov-row{display:flex;justify-content:space-between;gap:8px;padding:6px 0;border-bottom:1px solid var(--line);font-size:10px}.gov-row:last-child{border-bottom:0}.gov-row span{color:var(--muted)}.gov-tag{font:9px var(--mono);padding:2px 5px;border:1px solid var(--line);border-radius:4px}.gov-tag.ok{border-color:#286b4a;color:var(--ok)}.gov-tag.pending{border-color:#6b5420;color:var(--warn)}.audit-list{max-height:290px;overflow:auto}.audit-item{padding:8px 0;border-bottom:1px solid var(--line);font-size:10px}.audit-item b{font-family:var(--mono);font-weight:500}.audit-item span{display:block;color:var(--muted);font:9px var(--mono);margin-top:3px}.v7-close{float:right;background:transparent;border:1px solid var(--line);color:var(--muted);border-radius:6px;padding:4px 7px}.workspace-create{display:flex;gap:6px;margin-top:8px}.workspace-create input{flex:1;background:var(--panel2);border:1px solid var(--line);border-radius:6px;color:var(--text);padding:7px;font-size:10px}.v7-note{font-size:9px;color:var(--muted);line-height:1.45;margin-top:10px}.enterprise-context .role-badge{border-color:var(--accent);color:var(--accent2)}
@media(max-width:760px){.workspace-menu{left:10px;top:55px}.governance-panel{right:10px}}
'''
s=s.replace('</style>\n</head>',css+'</style>\n</head>')
html=r'''
<!-- ===== V7 Workspace / Governance UI ===== -->
<div id="workspaceMenu" class="workspace-menu" style="display:none">
  <div class="workspace-head"><small>Organization / Workspace</small><b>CoAgentics Research</b><span class="mini">Select the active research workspace</span></div>
  <div id="workspaceOptions"></div>
  <div class="workspace-create"><input id="newWorkspace" placeholder="New workspace name"><button class="action" onclick="createWorkspace()">Create</button></div>
  <div class="v7-note">Local workspace registry for this build. Enterprise identity, SSO, and centralized authorization require deployment services; this UI does not claim those capabilities locally.</div>
</div>
<div id="governancePanel" class="governance-panel" style="display:none">
  <button class="v7-close" onclick="closeGovernance()">×</button>
  <small>Platform governance</small><h2 style="margin:3px 0 0;font-family:var(--display)">Workspace Administration</h2>
  <div class="gov-grid">
    <div class="gov-card"><h3>Identity & access</h3>
      <div class="gov-row"><span>Current role</span><b id="govRole">Research Owner</b></div>
      <div class="gov-row"><span>RBAC enforcement</span><span class="gov-tag pending">DEPLOYMENT</span></div>
      <div class="gov-row"><span>SSO / identity provider</span><span class="gov-tag pending">DEPLOYMENT</span></div>
      <div class="gov-row"><span>Session</span><span class="gov-tag ok">LOCAL ACTIVE</span></div>
    </div>
    <div class="gov-card"><h3>Research integrity</h3>
      <div class="gov-row"><span>Experiment versioning</span><span class="gov-tag ok">ENABLED</span></div>
      <div class="gov-row"><span>Evidence provenance</span><span class="gov-tag ok">ENABLED</span></div>
      <div class="gov-row"><span>Claim approval</span><span class="gov-tag ok">GATED</span></div>
      <div class="gov-row"><span>Immutable run state</span><span class="gov-tag ok">DESIGN RULE</span></div>
    </div>
  </div>
  <div class="gov-card" style="margin-top:10px"><h3>Audit activity</h3><div id="auditList" class="audit-list"><div class="empty">Loading audit history…</div></div></div>
  <div class="v7-note">Audit entries record UI actions and experiment-state transitions available to this local workbench. Production deployment should bind these events to the organization's centralized audit service.</div>
</div>
'''
s=s.replace('</body>',html+'\n</body>')
js=r'''
/* ===== V7 Enterprise Workspace / Audit ===== */
const V7_KEY='coagentics-workspaces-v7';
let v7Workspaces=JSON.parse(localStorage.getItem(V7_KEY)||'null')||[
 {id:'dv026',name:'DV026 · Economic Behavior',description:'DARPA economic behavior research',role:'Research Owner',active:true},
 {id:'shared',name:'Shared Evidence',description:'Review and evidence artifacts',role:'Reviewer',active:false}
];
function saveV7(){localStorage.setItem(V7_KEY,JSON.stringify(v7Workspaces));}
function activeWorkspace(){return v7Workspaces.find(w=>w.active)||v7Workspaces[0];}
function renderWorkspaceMenu(){const root=$('workspaceOptions');if(!root)return;root.innerHTML=v7Workspaces.map(w=>`<button class="workspace-option ${w.active?'active':''}" onclick="selectWorkspace('${esc(w.id)}')"><span class="ws-icon">${w.active?'●':'○'}</span><span><b>${esc(w.name)}</b><span>${esc(w.description)}</span></span><em>${esc(w.role)}</em></button>`).join('');}
function openWorkspace(){renderWorkspaceMenu();$('workspaceMenu').style.display='block';$('governancePanel').style.display='none';}
function closeWorkspace(){if($('workspaceMenu'))$('workspaceMenu').style.display='none';}
function selectWorkspace(id){v7Workspaces=v7Workspaces.map(w=>({...w,active:w.id===id}));saveV7();const w=activeWorkspace();const btn=$('workspaceBtn');if(btn)btn.querySelector('b').textContent=w.name;const ctx=$('contextExperiment');if(ctx)ctx.textContent=w.name+' / current experiment';const role=$('govRole');if(role)role.textContent=w.role;closeWorkspace();toast('Workspace changed',w.name);audit('workspace.select',w.name);}
function createWorkspace(){const input=$('newWorkspace');const name=(input?.value||'').trim();if(!name)return;const id='ws-'+Date.now();v7Workspaces.push({id,name,description:'New research workspace',role:'Research Owner',active:false});saveV7();input.value='';renderWorkspaceMenu();audit('workspace.create',name);toast('Workspace created',name);}
async function audit(action,detail){try{await fetch('/api/platform/audit',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({action,detail,workspace:activeWorkspace()?.id||'unknown',actor:'local-researcher'})});}catch(_){} }
async function openGovernance(){closeWorkspace();$('governancePanel').style.display='block';try{const d=await get('/api/platform/audit');const list=d.events||[];$('auditList').innerHTML=list.length?list.slice(-30).reverse().map(e=>`<div class="audit-item"><b>${esc(e.action||'event')}</b> · ${esc(e.detail||'')}<span>${esc(e.timestamp||'')} · ${esc(e.workspace||'')} · ${esc(e.actor||'')}</span></div>`).join(''):'<div class="empty">No audit events yet.</div>';}catch(e){$('auditList').innerHTML='<div class="empty">Audit service unavailable.</div>';}}
function closeGovernance(){if($('governancePanel'))$('governancePanel').style.display='none';}
function v7Init(){const w=activeWorkspace();const btn=$('workspaceBtn');if(btn){btn.querySelector('b').textContent=w.name;btn.onclick=openWorkspace;}const auditBtn=$('auditBtn');if(auditBtn)auditBtn.onclick=openGovernance;const settings=$('settingsBtn');if(settings)settings.onclick=openGovernance;renderWorkspaceMenu();audit('session.open','Enterprise workbench opened');}
v7Init();
'''
s=s.replace('</script></body>',js+'</script></body>')
p.write_text(s)
