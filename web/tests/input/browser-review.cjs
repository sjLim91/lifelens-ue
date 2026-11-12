// Optional browser review: run against a freshly built local preview.
// Provide Playwright via LIFELENS_PLAYWRIGHT_MODULE when it is not installed locally.
const { chromium }=require(process.env.LIFELENS_PLAYWRIGHT_MODULE || 'playwright');
const assert=require('node:assert/strict');
const fs=require('node:fs');
(async()=>{
  const browser=await chromium.launch({executablePath:process.env.LIFELENS_CHROMIUM_PATH || '/usr/bin/chromium',headless:true,args:['--no-sandbox','--disable-dev-shm-usage','--disable-gpu']});
  const results=[];
  try{
    for(const spec of [
      {name:'phone',viewport:{width:393,height:852},hasTouch:true,isMobile:true},
      {name:'small-phone',viewport:{width:320,height:568},hasTouch:true,isMobile:true},
      {name:'landscape',viewport:{width:915,height:412},hasTouch:true,isMobile:true},
      {name:'desktop',viewport:{width:1440,height:1000},hasTouch:false,isMobile:false},
    ]){
      const context=await browser.newContext({...spec,reducedMotion:'reduce'});
      await context.addInitScript(()=>localStorage.setItem('lifelens.web.renderMode','legacy-canvas'));
      const page=await context.newPage(),errors=[];
      page.on('pageerror',e=>errors.push(e.message));
      await page.goto((process.env.LIFELENS_REVIEW_URL || 'http://127.0.0.1:4173/')+'index.html?ui-review='+spec.name,{waitUntil:'domcontentloaded'});
      await page.waitForFunction(()=>document.querySelector('#status')?.textContent?.includes('시뮬레이션 코어 연결됨'),null,{timeout:45000});

      assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'no horizontal page overflow');
      await page.screenshot({path:'/tmp/lifelens-ui-'+spec.name+'.png'});
      if(spec.hasTouch){
        const nav=page.getByRole('navigation',{name:'주요 관찰 메뉴'});
        assert(await nav.isVisible());
        assert.equal(await page.locator('.desktop-observer').count(),0,'mobile does not compute the hidden desktop inspector');
        await page.getByRole('button',{name:'일시정지',exact:true}).click();
        assert.equal(await page.getByRole('button',{name:'일시정지',exact:true}).getAttribute('aria-pressed'),'true');
        await page.getByRole('button',{name:'4× 관찰 속도',exact:true}).click();
        assert.equal(await page.getByRole('button',{name:'4× 관찰 속도',exact:true}).getAttribute('aria-pressed'),'true');
        await page.getByRole('button',{name:'일시정지',exact:true}).click();
        await nav.getByRole('button',{name:'주민',exact:true}).click();
        const dialog=page.getByRole('dialog',{name:'주민',exact:true});
        await dialog.waitFor();
        assert.equal(await dialog.locator('.resident-card').count(),4);
        assert.equal(await dialog.locator('[role=meter]').count(),20);
        assert((await dialog.locator('.resident-card').first().boundingBox()).height<245);
        await page.keyboard.press('Tab');
        assert(await page.evaluate(()=>document.activeElement.closest('dialog')?.open),'native dialog contains keyboard focus');
        await page.screenshot({path:'/tmp/lifelens-ui-'+spec.name+'-residents.png'});
        await dialog.locator('.resident-card').first().click();
        await page.getByRole('dialog').locator('.focused-life').waitFor();
        await page.keyboard.press('Escape');
        assert.equal(await page.locator('dialog.mobile-modal-backdrop').count(),0);
        await nav.getByRole('button',{name:'기록',exact:true}).click();
        await page.getByRole('dialog',{name:'최근 관찰'}).waitFor();
        await page.getByRole('button',{name:'팝업 닫기'}).click();
        await nav.getByRole('button',{name:'메뉴',exact:true}).click();
        const menu=page.getByRole('dialog',{name:'관찰 메뉴'});
        await menu.waitFor();
        await page.screenshot({path:'/tmp/lifelens-ui-'+spec.name+'-menu.png'});
        await menu.getByRole('button',{name:'월드 설정'}).click();
        const settings=page.getByRole('dialog',{name:'월드 설정'});
        await settings.waitFor();
        assert(await settings.locator('.mobile-modal-body').evaluate(el=>el.scrollWidth<=el.clientWidth),'settings controls fit narrow and landscape panels');
        assert.equal(await settings.locator('details[open]').count(),0);
        await settings.getByText('새 세계 · 시드로 다시 시작',{exact:true}).click();
        const seedBefore=await page.locator('.mobile-current-seed b').innerText();
        page.once('dialog',d=>d.dismiss());
        await settings.getByRole('button',{name:'새 월드',exact:true}).click();
        assert.equal(await page.locator('.mobile-current-seed b').innerText(),seedBefore,'dismissing restart keeps current world');
        await page.getByRole('button',{name:'팝업 닫기'}).click();
        if(spec.name==='phone'){
          const cdp=await context.newCDPSession(page);
          const touch=(type,pts)=>cdp.send('Input.dispatchTouchEvent',{type,touchPoints:pts.map(([id,x,y])=>({id,x,y,radiusX:1,radiusY:1,force:1}))});
          const pause=()=>page.waitForTimeout(70);
          const pixels = () => page.locator('#worldCanvas').evaluate(c=>c.toDataURL());
          const initial=await pixels();
          await touch('touchStart',[[1,150,320]]);
          await touch('touchMove',[[1,190,350]]); await pause();
          await touch('touchEnd',[]); await pause();
          const rotated=await pixels();
          assert.notEqual(rotated,initial,'one-finger orbit visibly changes the actual world');
          await touch('touchStart',[[1,110,320],[2,250,320]]);
          await touch('touchMove',[[1,135,335],[2,275,335]]); await pause();
          const panned=await pixels();
          assert.notEqual(rotated,panned,'sub-chunk pan is visibly applied to the actual fallback canvas');
          await touch('touchMove',[[1,110,335],[2,300,335]]);await pause();
          const zoomed=await pixels();assert.notEqual(zoomed,panned,'pinch outward changes scale');
          await touch('touchMove',[[1,130,335],[2,280,335]]);await pause();
          assert.notEqual(await pixels(),zoomed,'pinch inward changes scale');
          await touch('touchEnd',[[1,130,335]]);await pause();
          const oneFinger=await pixels();
          await touch('touchMove',[[1,170,355]]);await pause();
          assert.equal(await pixels(),oneFinger,'lifting one finger cannot switch to orbit');
          await touch('touchEnd',[]);await pause();
          assert.equal(await page.locator('dialog.mobile-modal-backdrop').count(),0,'gestures cannot accidentally open resident details');
          await nav.getByRole('button',{name:'메뉴',exact:true}).click();
          await page.setViewportSize({width:1440,height:900});
          await page.waitForTimeout(120);
          assert.equal(await page.locator('dialog.mobile-modal-backdrop').count(),0,'desktop transition releases modal inertness');
          assert(await page.locator('.desktop-observer').isVisible());
        }
      }else{
        assert(await page.locator('.desktop-observer').isVisible());
        assert.equal(await page.locator('.mobile-navigation').isVisible(),false);
      }
      assert.deepEqual(errors,[]);
      results.push({name:spec.name,passed:true,errors});
      await context.close();
    }
    fs.writeFileSync('/tmp/lifelens-ui-interaction-results.json',JSON.stringify(results,null,2));
    console.log(JSON.stringify(results));
  }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
