#ifndef HTTP_UI_H
#define HTTP_UI_H

/*
 *  Static HTML/CSS/JS page templates served by http_server.cpp.
 *  Kept here to keep the request handlers readable.
 */

#include <pgmspace.h>

static const char HTML_BEGIN[] PROGMEM = R"(
<!DOCTYPE HTML>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0">
    <title>WiFi Clock</title>
    <style>
      body { background-color: white; font-family: Arial, Helvetica, Sans-Serif; color: #000000; }
      .contain{width: 100%;}
      .center_div{margin:0 auto; max-width: 400px;position:relative;}
    </style>
  </head>
  <body>
)";

static const char HTML_END[] PROGMEM = "</body></html>";

static const char INDEX_HTML_0[] PROGMEM = R"(
<style>
  .btn_cfg{border:0;border-radius:0.3rem;color:#fff;line-height:1.4rem;font-size:0.8rem;margin:1ch;height:2rem;width:10rem;background-color:#ff3300;}
</style>
<style>
  #status{position:sticky;top:0;z-index:10;min-height:1.4rem;padding:0.4rem;margin-bottom:0.5rem;
    border-radius:0.3rem;font-size:0.85rem;display:none;text-align:left;}
  #status.ok{display:block;background:#d4edda;color:#155724;}
  #status.err{display:block;background:#f8d7da;color:#721c24;}
  #status.info{display:block;background:#e2e3e5;color:#383d41;}
</style>
<div class="contain">
  <div class="center_div">
  <div id="status"></div>
  <h1>WiFi Clock</h1>
)";

static const char INDEX_HTML_1[] PROGMEM = R"(
  <br>
  <label for="led">LED color: </label>
  <input type="color" id="led" value="#000000" oninput="setLed(this.value);">
  <br/><br/>
  <label for="bl">Brightness: </label>
  <input type="range" id="bl" min="0" max="100" value="100" oninput="setBl(this.value);">
  <br/><br/>
  <button class="btn_cfg" type="button" onclick="fetch('/flashbl');">Flash</button>
  <br/>
  <button class="btn_cfg" type="button" onclick="fetch('/flipscreen');">Flip Screen</button>
  <br/>
  <button class="btn_cfg" type="button" onclick="location.href='/selectap';">Configure WiFi</button>
  <br/>
  <hr>
  <label for="up">Upload image: </label>
  <input type="file" id="up" accept="image/*" onchange="uploadFile(this.files[0]);">
  <hr>
  <p>Gallery (click to display):</p>
  <div id="gal"></div>
  <hr>
  <a href='https://github.com/ujagaga/esp32_clock_face' target="_blank" rel="noopener noreferrer">Source code</a>
</div>
<style>
  #gal{display:flex;flex-wrap:wrap;gap:8px;justify-content:center;}
  #gal figure{margin:0;text-align:center;}
  #gal canvas{width:160px;height:auto;border:2px solid #ccc;border-radius:4px;display:block;cursor:pointer;}
  #gal figure.sel canvas{border-color:#ff3300;}
  #gal figcaption{font-size:0.7rem;word-break:break-all;width:160px;}
  #gal button.del{border:0;border-radius:0.3rem;color:#fff;background:#ff3300;font-size:0.7rem;margin-top:0.3rem;padding:0.2rem 0.6rem;cursor:pointer;}
</style>
<script>
  var PV_W=320, PV_H=172;
  function showStatus(msg, kind){
    var s=document.getElementById('status');
    s.textContent=msg;
    s.className=kind||'info';
  }
  function selectImg(name){ fetch('/setdisplay?img=' + encodeURIComponent(name)); markSel(name); }
  function markSel(name){
    var figs=document.querySelectorAll('#gal figure');
    for(var i=0;i<figs.length;i++){
      figs[i].className = (figs[i].dataset.name===name) ? 'sel' : '';
    }
  }
  function onSel(v){ fetch('/setdisplay?img=' + encodeURIComponent(v)); markSel(v); }
  function deleteImg(name){
    if(!confirm('Delete ' + name + '?')){ return; }
    fetch('/delete?name=' + encodeURIComponent(name)).then(function(r){
      return r.text().then(function(t){
        if(r.ok){
          showStatus('Deleted ' + name, 'ok');
          document.getElementById('gal').innerHTML=''; buildGallery();
        }else{
          showStatus('Delete failed: ' + t, 'err');
        }
      });
    }).catch(function(e){ showStatus('Delete error: ' + e.message, 'err'); });
  }
  function decode(ab, cv){
    var bytes=new Uint8Array(ab);
    var ctx=cv.getContext('2d');
    var img=ctx.createImageData(PV_W, PV_H);
    var d=img.data, n=PV_W*PV_H;
    for(var i=0;i<n;i++){
      var p=(bytes[2*i]<<8)|bytes[2*i+1];      // big-endian RGB565
      var r5=(p>>11)&0x1f, g6=(p>>5)&0x3f, b5=p&0x1f;
      d[4*i]  =(r5*255/31)|0;
      d[4*i+1]=(g6*255/63)|0;
      d[4*i+2]=(b5*255/31)|0;
      d[4*i+3]=255;
    }
    ctx.putImageData(img,0,0);
  }
  // Fetch and render images one at a time so the ESP32 streams a single file at once.
  function loadOne(names, idx){
    if(idx>=names.length){ return; }
    var name=names[idx];
    fetch('/getimage?name=' + encodeURIComponent(name)).then(function(r){
      if(!r.ok){ throw new Error('not found'); }
      return r.arrayBuffer();
    }).then(function(ab){
      var fig=document.createElement('figure');
      fig.dataset.name=name;
      var cv=document.createElement('canvas');
      cv.width=PV_W; cv.height=PV_H;
      cv.onclick=function(){ selectImg(name); };
      var cap=document.createElement('figcaption');
      cap.textContent=name;
      var del=document.createElement('button');
      del.textContent='Delete';
      del.className='del';
      del.onclick=function(){ deleteImg(name); };
      fig.appendChild(cv); fig.appendChild(cap); fig.appendChild(del);
      document.getElementById('gal').appendChild(fig);
      decode(ab, cv);
    }).catch(function(){}).then(function(){
      loadOne(names, idx+1);     // next image only after this one finishes
    });
  }
  // Mirror of the LCD clock face, fed by the device's time over WebSocket.
  var clockCanvas=null;
  function drawClock(hh, mm, ss, date){
    if(!clockCanvas){ return; }
    var ctx=clockCanvas.getContext('2d');
    ctx.fillStyle='#000'; ctx.fillRect(0,0,PV_W,PV_H);
    ctx.textAlign='center'; ctx.textBaseline='middle';
    if(hh===null){
      ctx.fillStyle='#ffff00';
      ctx.font='bold 20px monospace';
      ctx.fillText('Waiting NTP...', PV_W/2, PV_H/2);
      return;
    }
    var sep=(ss%2===0)?':':' ';
    ctx.fillStyle='#ffff00';
    ctx.font='bold 64px monospace';
    ctx.fillText(hh+sep+mm, PV_W/2, PV_H/2-12);
    ctx.fillStyle='#0000ff';
    ctx.font='bold 30px monospace';
    ctx.fillText(date, PV_W/2, PV_H/2+50);
  }
  // Device pushes time and active display over WebSocket (no polling).
  function startSocket(){
    var ws=new WebSocket('ws://'+location.hostname+':81/');
    ws.onopen=function(){ ws.send('{"GETDISP":""}'); };  // ask for current display
    ws.onmessage=function(e){
      var d; try{ d=JSON.parse(e.data); }catch(_){ return; }
      if(d.hasOwnProperty('TIME')){
        var p=d.TIME.split('|');
        if(p.length<4){ drawClock(null); }
        else{ drawClock(p[0], p[1], parseInt(p[2],10), p[3]); }
      }
      if(d.hasOwnProperty('DISP')){
        var sel=document.getElementById('disp');
        if(sel && sel.value!==d.DISP){ sel.value=d.DISP; }
        markSel(d.DISP);
      }
    };
    ws.onclose=function(){ setTimeout(startSocket, 3000); };  // auto-reconnect
  }
  function addClockTile(){
    var fig=document.createElement('figure');
    fig.dataset.name='clock';
    clockCanvas=document.createElement('canvas');
    clockCanvas.width=PV_W; clockCanvas.height=PV_H;
    clockCanvas.onclick=function(){ selectImg('clock'); };
    var cap=document.createElement('figcaption');
    cap.textContent='Clock';
    fig.appendChild(clockCanvas); fig.appendChild(cap);
    document.getElementById('gal').appendChild(fig);
    drawClock(null);   // "Waiting NTP..." until first WS push
  }
  function buildGallery(){
    addClockTile();
    fetch('/imagelist').then(function(r){return r.text();}).then(function(t){
      var names=t.split('|').filter(function(s){return s.length>0;});
      loadOne(names, 0);
    });
  }
  // Resize+crop to PV_W x PV_H, pack big-endian RGB565, POST to /upload.
  function uploadFile(file){
    if(!file){ return; }
    showStatus('Resizing ' + file.name + '...', 'info');
    var url=URL.createObjectURL(file);
    var im=new Image();
    im.onload=function(){
      URL.revokeObjectURL(url);
      var cv=document.createElement('canvas');
      cv.width=PV_W; cv.height=PV_H;
      var ctx=cv.getContext('2d');
      // cover: scale to fill, center-crop
      var s=Math.max(PV_W/im.width, PV_H/im.height);
      var dw=im.width*s, dh=im.height*s;
      ctx.drawImage(im, (PV_W-dw)/2, (PV_H-dh)/2, dw, dh);
      var d=ctx.getImageData(0,0,PV_W,PV_H).data;
      var n=PV_W*PV_H, out=new Uint8Array(n*2);
      for(var i=0;i<n;i++){
        var r=d[4*i], g=d[4*i+1], b=d[4*i+2];
        var p=((r&0xf8)<<8)|((g&0xfc)<<3)|(b>>3);
        out[2*i]=(p>>8)&0xff;        // big-endian
        out[2*i+1]=p&0xff;
      }
      // strip extension, force .bin
      var base=file.name.replace(/\.[^.]+$/,'').replace(/[^A-Za-z0-9_-]/g,'_').substring(0,28);
      var fname=base+'.bin';
      var fd=new FormData();
      fd.append('f', new Blob([out], {type:'application/octet-stream'}), fname);
      showStatus('Uploading ' + fname + ' (' + (out.length) + ' bytes)...', 'info');
      fetch('/upload', {method:'POST', body:fd}).then(function(r){
        return r.text().then(function(t){
          if(r.ok){
            showStatus('Uploaded ' + fname, 'ok');
            document.getElementById('gal').innerHTML=''; buildGallery();
          }else{
            showStatus('Upload failed: ' + t, 'err');
          }
        });
      }).catch(function(e){ showStatus('Upload error: ' + e.message, 'err'); });
    };
    im.onerror=function(){ URL.revokeObjectURL(url); showStatus('Cannot read image file', 'err'); };
    im.src=url;
  }
  function setLed(v){ fetch('/setled?c=' + v.substring(1)); }
  function setBl(v){ fetch('/setbl?v=' + v); }
  buildGallery();
  startSocket();   // device pushes time + active display over WebSocket
</script>
)";

static const char APLIST_HTML_0[] PROGMEM = R"(
<style>
  .c{text-align: center;}
  div,input{padding:5px;font-size:1em;}
  input{width:95%;}
  body{text-align: left;}
  button{width:100%;border:0;border-radius:0.3rem;color:#fff;line-height:2.4rem;font-size:1.2rem;height:40px;background-color:#1fa3ec;}
  .q{float: right;width: 64px;text-align: right;}
  .radio{width:2em;}
  #vm{width:100%;height:50vh;overflow-y:auto;margin-bottom:1em;}
</style>
</head><body>
  <div class="contain">
    <div class="center_div">
)";

static const char APLIST_HTML_1[] PROGMEM = R"(
      <h1 id='ttl'>Networks found:</h1>
      <div id='vm'>
)";

static const char APLIST_HTML_2[] PROGMEM = R"(
      </div>
      <form method='get' action='wifisave'>
        <button type='button' onclick='refresh();'>Rescan</button><br/><br/>
        <input id='s' name='s' length=32 placeholder='SSID (Leave blank for AP mode)'><br>
        <input id='p' name='p' length=32 placeholder='Password'><br>
        <br><button type='submit'>Save</button>
      </form>
     </div>
  </div>
<script>
  function c(l){
    document.getElementById('s').value=l.innerText||l.textContent;
    document.getElementById('p').focus();
  }

  var cn=new WebSocket('ws://'+location.hostname+':81/');
  cn.onopen=function(){
    cn.send('{"APLIST":""}');
  }
  cn.onmessage=function(e){
    var data=JSON.parse(e.data);
    if(data.hasOwnProperty('APLIST')){
      rsp=data.APLIST.split('|');
      document.getElementById('vm').innerHTML='';
      for(var i=0;i<(rsp.length);i++){
        document.getElementById('vm').innerHTML+='<span>'+(i+1)+": </span><a href='#p' onclick='c(this)'>" + rsp[i] + '</a><br>';
      }
      if(!document.getElementById('vm').innerHTML.replace(/\\s/g,'').length){
        document.getElementById('ttl').innerHTML='No networks found.'
      }
    }
  };
  function refresh(){
    document.getElementById('vm').innerHTML='Please wait...'
    cn.send('{"APLIST":""}');
  }
</script>
)";

static const char REDIRECT_HTML[] PROGMEM = R"(
<p id="tmr"></p>
<script>
  var c=6;
  function count(){
    var tmr=document.getElementById('tmr');
    if(c>0){
      c--;
      tmr.innerHTML="You will be redirected to home page in "+c+" seconds.";
      setTimeout('count()',1000);
    }else{
      window.location.href="/";
    }
  }
  count();
</script>
)";

#endif
