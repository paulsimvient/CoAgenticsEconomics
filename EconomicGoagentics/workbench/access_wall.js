(function(){
  if(document.cookie.indexOf('coagentics_gate=ok')>=0) return;
  if(localStorage.getItem('coagentics_gate')==='ok') return;
  const wall=document.createElement('div');
  wall.setAttribute('id','coagentics-access-wall');
  wall.innerHTML='<style>#coagentics-access-wall{position:fixed;inset:0;z-index:9999;background:#f7f2e8;color:#102f4c;display:grid;place-items:center;padding:6vh 6vw;font-family:Georgia,serif}#coagentics-access-wall .card{width:min(560px,100%)}#coagentics-access-wall .eyebrow{font-family:Arial,sans-serif;letter-spacing:.18em;text-transform:uppercase;font-size:13px;color:#a57b23;font-weight:700;margin-bottom:1.1rem}#coagentics-access-wall h1{margin:0 0 .9rem;font-size:clamp(42px,6vw,64px);line-height:.95;font-weight:500}#coagentics-access-wall .rule{width:72px;height:2px;background:#a57b23;margin:0 0 1.4rem}#coagentics-access-wall p{margin:0 0 1.6rem;font-size:21px;line-height:1.35;max-width:28ch}#coagentics-access-wall form{display:grid;gap:.7rem}#coagentics-access-wall label{font-family:Arial,sans-serif;font-size:12px;letter-spacing:.12em;text-transform:uppercase;font-weight:700}#coagentics-access-wall input{width:100%;padding:.95rem 1rem;border:1px solid rgba(16,47,76,.22);border-radius:12px;font-size:18px;font-family:Arial,sans-serif}#coagentics-access-wall button{margin-top:.4rem;padding:.95rem 1.1rem;border:0;border-radius:12px;background:#102f4c;color:#f7f2e8;font-family:Arial,sans-serif;font-size:16px;font-weight:700;cursor:pointer}#coagentics-access-wall .fine{font-family:Arial,sans-serif;font-size:13px;color:rgba(16,47,76,.62);max-width:42ch}#coagentics-access-wall .err{display:none;font-family:Arial,sans-serif;color:#a33;font-size:14px}</style><div class="card"><div class="eyebrow">INVIA Digital · CoAgentics</div><h1>Request access</h1><div class="rule"></div><p>Enter your email to open this page.</p><form id="coagentics-gate-form"><label for="coagentics-email">Email</label><input id="coagentics-email" type="email" required maxlength="200" autocomplete="email" placeholder="you@organization.com"><div class="err" id="coagentics-err">Enter a valid email.</div><button type="submit">Continue</button></form><p class="fine">We’ll use this to follow up. Nothing here is a public dataset.</p></div>';
  document.documentElement.appendChild(wall);
  wall.addEventListener('click',function(e){e.stopPropagation()});
  wall.addEventListener('keydown',function(e){e.stopPropagation()});
  document.getElementById('coagentics-gate-form').addEventListener('submit',async function(e){
    e.preventDefault(); e.stopPropagation();
    const email=document.getElementById('coagentics-email').value.trim();
    if(!/^[^\s@]+@[^\s@]+\.[A-Za-z]{2,}$/.test(email)){
      document.getElementById('coagentics-err').style.display='block';
      return;
    }
    try{
      await fetch('/api/access/email',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({email:email,next:location.pathname})});
    }catch(err){}
    localStorage.setItem('coagentics_gate','ok');
    document.cookie='coagentics_gate=ok; Path=/; Max-Age=2592000; SameSite=Lax';
    wall.remove();
  });
})();
