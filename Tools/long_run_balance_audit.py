#!/usr/bin/env python3
"""LifeLens long-run balance audit runner/reporter.

This tool never changes simulation state or balance rules. It runs the
Core-owned audit executable, parses factual AUDIT_* records, compares seeds and
checkpoints, and emits JSON/CSV/Markdown artifacts. "Regression" findings are
limited to deterministic/structural invariants; no preferred balance target is
hard-coded.
"""
from __future__ import annotations
import argparse, csv, glob, hashlib, json, math, os, re, subprocess, sys
from collections import defaultdict
from pathlib import Path
from typing import Any, Iterable

LINE_RE = re.compile(r"^(AUDIT_[A-Z_]+)\s+(.*)$")

def scalar(value: str) -> Any:
    if value in {"nan", "-nan", "inf", "-inf"}:
        return value
    try:
        if any(ch in value for ch in ".eE"):
            return float(value)
        return int(value)
    except ValueError:
        return value

def parse_line(line: str, source: str = "") -> dict[str, Any] | None:
    m = LINE_RE.match(line.strip())
    if not m:
        return None
    record: dict[str, Any] = {"kind": m.group(1), "source": source}
    for token in m.group(2).split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        record[key] = scalar(value)
    return record

def parse_logs(paths: Iterable[Path]) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for path in paths:
        with path.open("r", encoding="utf-8", errors="replace") as fh:
            for line in fh:
                item = parse_line(line, str(path))
                if item:
                    records.append(item)
    return records

def keyed(records: list[dict[str, Any]], kind: str) -> dict[tuple[int, int], dict[str, Any]]:
    out = {}
    for r in records:
        if r.get("kind") != kind or "seed" not in r or "day" not in r:
            continue
        out[(int(r["seed"]), int(r["day"]))] = r
    return out

def by_kind(records: list[dict[str, Any]], kind: str) -> list[dict[str, Any]]:
    return [r for r in records if r.get("kind") == kind]

def pct(record: dict[str, Any], names: list[str]) -> float:
    return sum(float(record.get(name + "Pct", 0.0) or 0.0) for name in names)

def first_day(records: list[dict[str, Any]], pred) -> int | None:
    days = sorted({int(r["day"]) for r in records if "day" in r and pred(r)})
    return days[0] if days else None

def dominant_death(health: dict[str, Any]) -> tuple[str, int]:
    values = {
        "illness": int(health.get("deathsIllness", 0) or 0),
        "accident": int(health.get("deathsAccident", 0) or 0),
        "exposure": int(health.get("deathsExposure", 0) or 0),
        "deprivation": int(health.get("deathsDeprivation", 0) or 0),
        "other": int(health.get("deathsOther", 0) or 0),
    }
    return max(values.items(), key=lambda kv: (kv[1], kv[0]))

def issue(severity: str, code: str, symptom: str, first: Any, direct: str,
          upstream: str, impact: str, seeds: list[int], layer: str,
          difficulty: str, risk: str) -> dict[str, Any]:
    return {
        "severity": severity, "code": code, "symptom": symptom,
        "firstObserved": first, "directCause": direct, "upstreamCause": upstream,
        "affectedSystems": impact, "seeds": seeds, "authoritativeLayer": layer,
        "difficulty": difficulty, "crossSystemRisk": risk,
    }

def derive(records: list[dict[str, Any]]) -> dict[str, Any]:
    checkpoints = keyed(records, "AUDIT_CHECKPOINT")
    times = keyed(records, "AUDIT_TIME")
    worlds = keyed(records, "AUDIT_WORLD_GROWTH")
    settlements = keyed(records, "AUDIT_SETTLEMENT")
    healths = keyed(records, "AUDIT_HEALTH")
    civs = keyed(records, "AUDIT_CIVILIZATION")
    events = keyed(records, "AUDIT_EVENTS")
    resources = by_kind(records, "AUDIT_RESOURCE")
    needs = by_kind(records, "AUDIT_NEED")
    facilities = keyed(records, "AUDIT_FACILITY")
    seeds = sorted({int(r["seed"]) for r in records if "seed" in r})
    days = sorted({int(r["day"]) for r in records if "day" in r})

    findings: list[dict[str, Any]] = []
    observability = by_kind(records, "AUDIT_OBSERVABILITY")
    if observability:
        findings.append(issue("P3","authoritative-observability-gaps",
            "일부 요구 지표는 현재 Core read-model이 minute/material 단위 authority를 직접 노출하지 않아 event/assignment evidence로만 측정",
            1,
            "migration 전용 minute phase, material별 inter-settlement transfer, explicit spoil/loss 및 facility abandonment state가 별도 authority DTO로 노출되지 않음",
            "Web 추론이나 임의 계산으로 메우지 않는 것이 simulation truth 계약에 맞음",
            "진단 정밀도",seeds,"LifeLensCore observer/read-model layer","하","낮음"))
    extinct = [s for s in seeds if (s, max(days, default=0)) in checkpoints and int(checkpoints[(s,max(days))].get("living",0)) == 0]
    if extinct:
        findings.append(issue("P0","population-extinction",
            "day1000까지 생존 인구가 0이 되는 deterministic seed가 존재",
            {s:first_day([r for r in checkpoints.values() if int(r["seed"])==s],lambda r:int(r.get("living",0))==0) for s in extinct},
            "사망 누적이 출생/세대교체보다 먼저 전체 생활 인구를 소진",
            "Health/Needs/가족·돌봄 causal chain을 seed별로 추적해야 함",
            "가족·교육·기관·경제·교역·세대교체 전체",extinct,
            "LifeLensCore population/health/family authority","중~상","매우 높음"))

    zero_social = []
    for s in seeds:
        relevant = [r for (ss,d),r in times.items() if ss==s and d>=30]
        if relevant and all(pct(r,["social","datingFamily","parentingCare","teachingLearning","institutionEconomy","trade"]) == 0 for r in relevant):
            zero_social.append(s)
    if zero_social:
        findings.append(issue("P1","social-family-starvation",
            "day30 이후 사회/가족/교육/기관/교역 활동시간이 구조적으로 0",
            30,"Core presentation/activity truth에서 해당 활동이 한 번도 확보되지 않음",
            "생존/자원/이동/utility 선점 또는 prerequisite 불충족",
            "관계·연애·가족·문명",zero_social,"LifeLensCore decision/activity authority","중","높음"))

    trade_stuck=[]
    for s in seeds:
        k=(s,max(days,default=0))
        if k in settlements:
            r=settlements[k]
            if int(r.get("tradeDepartures",0))>0 and int(r.get("tradeReturns",0))<int(r.get("tradeDepartures",0)):
                trade_stuck.append(s)
    if trade_stuck:
        findings.append(issue("P1","trade-journey-incomplete",
            "교역 출발 대비 귀환이 누락되어 장기 여정이 완결되지 않음",
            {s:int(events.get((s,max(days)),{}).get("firstTradeDeparture",-1)) for s in trade_stuck},
            "출발한 trade mission의 exchange/return completion evidence 부족",
            "#642가 다루는 trade journey persistence 영역과 연관 가능하나 본 PR은 진단만 수행",
            "교역·정착지 노동력·가족 안정성",trade_stuck,"LifeLensCore trade journey authority (#642 lane)","중","높음"))

    illness_without_care=[]
    for s in seeds:
        k=(s,max(days,default=0))
        h=healths.get(k,{})
        if int(h.get("deathsIllness",0) or 0)>0 and int(h.get("careEvents",0) or 0)==0:
            illness_without_care.append(s)
    if illness_without_care:
        findings.append(issue("P1","illness-mortality-without-care",
            "질병 사망이 발생했지만 HealthCare action evidence가 0",
            {s:first_day([r for r in healths.values() if int(r["seed"])==s],lambda r:int(r.get("deathsIllness",0) or 0)>0) for s in illness_without_care},
            "질병 진행 중 실제 caregiver HealthCare presentation이 관측되지 않음",
            "caregiver 가용성·돌봄 선택·생존 선점·접근성 중 선행 병목을 causal chain으로 확인해야 함",
            "건강·가족·노동력·세대교체",illness_without_care,"LifeLensCore health/parenting authority","중","높음"))

    infrastructure_zero=[]
    infrastructure_first={}
    for s in seeds:
        for d in days:
            c=checkpoints.get((s,d),{}); fac=facilities.get((s,d),{})
            if int(c.get("living",0) or 0)>0 and int(fac.get("total",0) or 0)>0 and int(fac.get("operational",0) or 0)==0:
                infrastructure_zero.append(s); infrastructure_first[s]=d; break
    infrastructure_zero=sorted(set(infrastructure_zero))
    if infrastructure_zero:
        findings.append(issue("P1","living-population-without-operational-facilities",
            "생존 주민이 남아 있는 checkpoint에서 operational facility가 0",
            infrastructure_first,
            "건설된 시설의 내구/수리/재건 공급이 생활 인구보다 먼저 소진",
            "수리 자원·노동시간·survival preemption·시설 선택의 선행 병목 확인 필요",
            "생존·생산·위생·정착지",infrastructure_zero,"LifeLensCore facility/repair authority","중","높음"))

    settlement_zero=[]
    settlement_first={}
    for s in seeds:
        for d in days:
            c=checkpoints.get((s,d),{}); st=settlements.get((s,d),{})
            if int(c.get("living",0) or 0)>0 and int(st.get("settlements",0) or 0)>0 and int(st.get("activeSettlements",0) or 0)==0:
                settlement_zero.append(s); settlement_first[s]=d; break
    settlement_zero=sorted(set(settlement_zero))
    if settlement_zero:
        findings.append(issue("P1","living-population-without-active-settlement",
            "생존 주민이 있으나 active settlement가 0인 checkpoint가 존재",
            settlement_first,
            "정착지 service footprint/시설 vitality가 생활 인구 유지보다 먼저 붕괴",
            "시설 붕괴·이주 실패·자원 접근성의 선행 순서를 확인해야 함",
            "정착·이주·교역·가족 안정성",settlement_zero,"LifeLensCore settlement/facility authority","중","높음"))

    chunk_waste=[]
    for s in seeds:
        k=(s,max(days,default=0))
        if k in worlds:
            r=worlds[k]
            if int(r.get("explorationSuccess",0))>0 and int(r.get("usedDiscoveredResourceNodes",0))==0:
                chunk_waste.append(s)
    if chunk_waste:
        findings.append(issue("P1","exploration-without-followup",
            "탐험 성공은 있으나 탐험으로 materialize된 자원 후속 사용이 0",
            {s:first_day([r for r in worlds.values() if int(r["seed"])==s],lambda r:int(r.get("explorationSuccess",0))>0) for s in chunk_waste},
            "탐험 결과가 gather/settlement use로 이어지지 않음",
            "scarcity target 선택·이동비용·resource access·utility 재계획 확인 필요",
            "world growth·성능·자원경제",chunk_waste,"LifeLensCore exploration/resource authority","중","중~높음"))

    dead_knowledge=[]
    for s in seeds:
        k=(s,max(days,default=0))
        if k in civs and int(civs[k].get("neverUsedKnowledge",0))>0:
            dead_knowledge.append(s)
    if dead_knowledge:
        findings.append(issue("P2","never-used-knowledge",
            "발견/보유됐지만 successful use가 한 번도 없는 knowledge가 존재",
            {s:first_day([r for r in civs.values() if int(r["seed"])==s],lambda r:int(r.get("neverUsedKnowledge",0))>0) for s in dead_knowledge},
            "knowledge discovery 이후 실제 capability/action use evidence가 없음",
            "prerequisite·자원·시설·utility·전승 중 어느 단계가 막히는지 technique별 lag 확인",
            "기술·전문화·기관·경제",dead_knowledge,"LifeLensCore civilization authority","중","중"))

    unused_fac=[]
    for s in seeds:
        k=(s,max(days,default=0))
        if k in facilities and int(facilities[k].get("operational",0))>0 and int(facilities[k].get("unusedOperational",0))==int(facilities[k].get("operational",0)):
            unused_fac.append(s)
    if unused_fac:
        findings.append(issue("P2","all-operational-facilities-unused",
            "운영 가능한 시설이 존재하지만 persistent usage evidence가 전부 0",
            {s:first_day([r for r in facilities.values() if int(r["seed"])==s],lambda r:int(r.get("operational",0))>0) for s in unused_fac},
            "시설 completion과 실제 use가 연결되지 않음","facility demand/selection/use recording 확인 필요",
            "건설·생존·생산",unused_fac,"LifeLensCore facility authority","중","중"))

    resource_summary={}
    for s in seeds:
        rr=[r for r in resources if int(r.get("seed",-1))==s and int(r.get("day",-1))==max(days,default=0)]
        rr.sort(key=lambda r:(-int(r.get("shortageSampleMinutes",0)),str(r.get("material",""))))
        resource_summary[str(s)]=[{"material":r.get("material"),"shortageSampleMinutes":r.get("shortageSampleMinutes",0),
                                  "accessibleWorld":r.get("accessibleWorld",0),"carried":r.get("carried",0),"stored":r.get("stored",0),
                                  "desiredUnits":r.get("desiredUnits",0),"societyAvailableUnits":r.get("societyAvailableUnits",0),
                                  "deficitUnits":r.get("deficitUnits",0),"demand01":r.get("demand01",0)}
                                 for r in rr[:5]]

    timeline={}
    for s in seeds:
        ev=events.get((s,max(days,default=0)),{})
        tl=[]
        mapping=[("firstCriticalNeed","first critical Hunger/Thirst"),("firstCriticalPreemption","critical survival preemption"),("firstChunkGrowth","chunk growth"),
                 ("firstSecondSettlement","second active settlement"),("firstIllness","illness"),
                 ("firstMarriage","marriage"),("firstPregnancy","pregnancy"),("firstBirth","birth"),
                 ("firstTradeDeparture","trade departure"),("firstTradeExchange","trade exchange"),
                 ("firstTradeReturn","trade return"),("firstDeath","death")]
        for key,label in mapping:
            v=int(ev.get(key,-1) or -1)
            if v>=0: tl.append({"minute":v,"day":round(v/1440,3),"event":label})
        for r in resources:
            if int(r.get("seed",-1))!=s or int(r.get("shortageSampleMinutes",0))<=0: continue
            tl.append({"minute":int(r["day"])*1440,"day":int(r["day"]),"event":f"{r.get('material')} shortage observed by checkpoint"})
        tl.sort(key=lambda x:(x["minute"],x["event"]))
        timeline[str(s)]=tl

    dominant={}
    for s in seeds:
        h=healths.get((s,max(days,default=0)),{})
        dominant[str(s)]={"cause":dominant_death(h)[0],"count":dominant_death(h)[1]}

    return {
        "seeds":seeds,"checkpoints":days,"findings":findings,
        "resourceBottlenecks":resource_summary,"causalTimeline":timeline,
        "dominantDeathCause":dominant,
    }

def flatten_csv(records: list[dict[str, Any]], path: Path) -> None:
    keys=["kind","source","seed","day"]+sorted({k for r in records for k in r if k not in {"kind","source","seed","day"}})
    with path.open("w",newline="",encoding="utf-8") as fh:
        w=csv.DictWriter(fh,fieldnames=keys,extrasaction="ignore");w.writeheader()
        for r in records:w.writerow(r)

def md_table(headers: list[str], rows: list[list[Any]]) -> str:
    def esc(v:Any)->str:return str(v).replace("|","\\|")
    return "| "+" | ".join(headers)+" |\n| "+" | ".join(["---"]*len(headers))+" |\n"+"\n".join("| "+" | ".join(esc(v) for v in row)+" |" for row in rows)

def render_markdown(records: list[dict[str, Any]], derived: dict[str, Any], base_sha: str="", head_sha: str="") -> str:
    cp=keyed(records,"AUDIT_CHECKPOINT");tm=keyed(records,"AUDIT_TIME");wd=keyed(records,"AUDIT_WORLD_GROWTH")
    st=keyed(records,"AUDIT_SETTLEMENT");hl=keyed(records,"AUDIT_HEALTH");cv=keyed(records,"AUDIT_CIVILIZATION");fc=keyed(records,"AUDIT_FACILITY")
    survival_actions=by_kind(records,"AUDIT_SURVIVAL_ACTION")
    seeds=derived["seeds"];days=derived["checkpoints"];last=max(days,default=0)
    out=["# LifeLens 전 시스템 장기 밸런스 / 인과 감사","",
         f"- 기준 base: `{base_sha or '기록 없음'}`",f"- 감사 HEAD: `{head_sha or '기록 없음'}`",
         "- 원칙: simulation truth/밸런스/save format을 변경하지 않고 Core read-model·persistent state·event 순서만 계측한다.",
         f"- seeds: {', '.join(map(str,seeds))}",f"- checkpoints: {', '.join(map(str,days))}일","",
         "## 1. 판정 요약",""]
    if derived["findings"]:
        for f in derived["findings"]:
            out.append(f"- **{f['severity']} {f['code']}** — {f['symptom']} (seed: {', '.join(map(str,f['seeds']))})")
    else: out.append("- 구조적 P0/P1 invariant 위반은 계측 범위에서 발견되지 않았다.")
    out+=["","## 2. Seed / checkpoint 비교",""]
    rows=[]
    for s in seeds:
        for d in days:
            c=cp.get((s,d),{});t=tm.get((s,d),{});w=wd.get((s,d),{});q=st.get((s,d),{})
            if not c:continue
            rows.append([s,d,c.get("living"),c.get("totalPopulation"),round(float(t.get("survivalPct",0)),2),
                         round(pct(t,["social","datingFamily","parentingCare","teachingLearning","institutionEconomy"]),2),
                         w.get("chunks"),c.get("facilities"),q.get("activeSettlements")])
    out.append(md_table(["seed","day","living","population","survival %","social/family/civ %","chunks","facilities","active settlements"],rows))
    out+=["","## 3. day1000 활동시간 예산",""]
    rows=[]
    for s in seeds:
        t=tm.get((s,last),{})
        if t: rows.append([s]+[round(float(t.get(n+"Pct",0)),2) for n in
             ["survival","gatherResource","storageLogistics","constructionRepair","productionCrafting","cultivation","social","datingFamily","parentingCare","teachingLearning","institutionEconomy","exploration","migration","trade","idleWaiting"]])
    out.append(md_table(["seed","survival","gather","storage","build/repair","production","cultivation","social","dating/family","care","teach","institution","explore","migration","trade","idle"],rows))
    out+=["","## 4. 생존 행동 세부 예산",""]
    rows=[]
    for s in seeds:
        for r in survival_actions:
            if int(r.get("seed",-1))==s and int(r.get("day",-1))==last:
                rows.append([s,r.get("action"),r.get("starts"),r.get("completions"),r.get("minutes"),r.get("travelDistance"),round(float(r.get("avgTravelPerCompletion",0)),2)])
    out.append(md_table(["seed","action","starts","completions","minutes","travel","avg travel/completion"],rows))
    out+=["","## 5. Resource bottleneck",""]
    for s in seeds:
        items=derived["resourceBottlenecks"].get(str(s),[])
        out.append(f"- seed {s}: "+(", ".join(f"{x['material']} shortageSample={x['shortageSampleMinutes']}min demand={x['desiredUnits']} societyStock={x['societyAvailableUnits']} deficit={x['deficitUnits']} worldAccessible={x['accessibleWorld']}" for x in items) if items else "기록 없음"))
    out+=["","## 6. Exploration / chunk growth",""]
    rows=[]
    for s in seeds:
        w=wd.get((s,last),{})
        if w: rows.append([s,w.get("explorationAttempts"),w.get("explorationSuccess"),w.get("criticalExploration"),w.get("ordinaryExploration"),w.get("chunks"),w.get("materializedAfterStart"),w.get("usedChunks"),round(float(w.get("discoveryFollowupRate",0)),3),w.get("explorationP95Distance"),w.get("explorationMaxDistance")])
    out.append(md_table(["seed","attempts","success","critical","ordinary","chunks","new chunks","used chunks","resource follow-up","p95 distance","max distance"],rows))
    out+=["","## 7. Population / health",""]
    rows=[]
    for s in seeds:
        c=cp.get((s,last),{});h=hl.get((s,last),{});dom=derived["dominantDeathCause"].get(str(s),{})
        if c:rows.append([s,c.get("living"),c.get("births"),c.get("generationCount"),h.get("illnessIncidence"),h.get("recoveries"),h.get("careEvents"),h.get("contaminationExposureResidents"),h.get("contaminatedWaterNodes"),dom.get("cause"),dom.get("count")])
    out.append(md_table(["seed","living","births","generations","illness","recovery","care","exposed residents","contaminated water nodes","dominant death","count"],rows))
    out+=["","## 8. Knowledge / civilization dead-system",""]
    rows=[]
    for s in seeds:
        c=cv.get((s,last),{})
        if c:rows.append([s,c.get("knownTechniqueTypes"),c.get("usedKnowledge"),c.get("neverUsedKnowledge"),c.get("specializedResidents"),c.get("educators"),c.get("producers"),c.get("caregivers"),c.get("storekeepers"),c.get("activeInstitutions"),c.get("durableRecords"),c.get("barterExchangeFacts")])
    out.append(md_table(["seed","known","used","never used","specialized","educators","producers","caregivers","storekeepers","institutions","records","barter facts"],rows))
    out+=["","## 9. Facilities / performance",""]
    rows=[]
    for s in seeds:
        c=cp.get((s,last),{});f=fc.get((s,last),{})
        if c:rows.append([s,c.get("elapsedMs"),c.get("snapshotBytes"),c.get("resourceNodes"),c.get("chunks"),c.get("facilities"),f.get("unusedOperational"),round(float(f.get("unusedOperationalRatio",0)),3)])
    out.append(md_table(["seed","elapsed ms","snapshot bytes","resource nodes","chunks","facilities","unused op","unused ratio"],rows))
    out+=["","## 10. Causal timeline",""]
    for s in seeds:
        out.append(f"### seed {s}")
        tl=derived["causalTimeline"].get(str(s),[])
        out.extend([f"- day {x['day']}: {x['event']}" for x in tl] or ["- 기록된 causal milestone 없음"])
    out+=["","## 11. 문제 분류 상세",""]
    for f in derived["findings"]:
        out += [f"### {f['severity']} — {f['code']}",
                f"- 증상: {f['symptom']}",f"- 최초 발생: {f['firstObserved']}",
                f"- 직접 원인: {f['directCause']}",f"- 상위 원인: {f['upstreamCause']}",
                f"- 영향 시스템: {f['affectedSystems']}",f"- 재현 seed: {f['seeds']}",
                f"- authoritative layer: {f['authoritativeLayer']}",f"- 수정 난이도: {f['difficulty']}",
                f"- 타 시스템 위험: {f['crossSystemRisk']}",""]
    out+=["## 12. Regression 판정 원칙","",
          "- 결정론 byte/metric mismatch, crash, 저장 불가 같은 계약 위반만 hard failure로 사용한다.",
          "- 사회시간 %, 출산 수, 시설 수, chunk 수 등에 임의의 '좋은 숫자'를 하드코딩하지 않는다.",
          "- 밸런스 이상은 seed/checkpoint 추세와 실제 event/state 순서를 근거로 분류한다.",""]
    return "\n".join(out)

def report(args) -> int:
    paths=[]
    for pattern in args.logs:
        matches=glob.glob(pattern,recursive=True)
        paths.extend(Path(p) for p in matches if Path(p).is_file())
    paths=sorted(set(paths))
    if not paths:
        print("audit log not found",file=sys.stderr);return 2
    records=parse_logs(paths);derived=derive(records)
    out=Path(args.out_dir);out.mkdir(parents=True,exist_ok=True)
    (out/"long-run-balance-audit.json").write_text(json.dumps({"metadata":{"baseSha":args.base_sha,"headSha":args.head_sha},"derived":derived,"records":records},ensure_ascii=False,indent=2),encoding="utf-8")
    flatten_csv(records,out/"long-run-balance-audit.csv")
    (out/"long-run-balance-audit.md").write_text(render_markdown(records,derived,args.base_sha,args.head_sha),encoding="utf-8")
    print(f"parsedLogs={len(paths)} records={len(records)} seeds={','.join(map(str,derived['seeds']))} checkpoints={','.join(map(str,derived['checkpoints']))}")
    for f in derived["findings"]: print(f"FINDING {f['severity']} {f['code']} seeds={','.join(map(str,f['seeds']))}")
    return 0

def normalize_log(path: Path) -> list[str]:
    out=[]
    for line in path.read_text(encoding="utf-8",errors="replace").splitlines():
        if not line.startswith("AUDIT_"):continue
        if line.startswith("AUDIT_CHECKPOINT "):
            line=re.sub(r"\selapsedMs=\S+","",line)
        out.append(line)
    return out

def determinism(args)->int:
    a,b=Path(args.first),Path(args.second)
    na,nb=normalize_log(a),normalize_log(b)
    ok=na==nb
    print(f"metricIdentity={'PASS' if ok else 'FAIL'} linesA={len(na)} linesB={len(nb)}")
    if not ok:
        for i,(x,y) in enumerate(zip(na,nb)):
            if x!=y:
                print(f"firstMetricMismatchLine={i+1}\nA={x}\nB={y}");break
    snaps_ok=True
    if args.snapshot_a and args.snapshot_b:
        pa,pb=Path(args.snapshot_a),Path(args.snapshot_b)
        ha=hashlib.sha256(pa.read_bytes()).hexdigest();hb=hashlib.sha256(pb.read_bytes()).hexdigest()
        snaps_ok=ha==hb
        print(f"snapshotByteIdentity={'PASS' if snaps_ok else 'FAIL'} shaA={ha} shaB={hb}")
    return 0 if ok and snaps_ok else 1

def run(args)->int:
    out=Path(args.out_dir);out.mkdir(parents=True,exist_ok=True)
    seeds=[int(x) for x in args.seeds.split(",") if x.strip()]
    for seed in seeds:
        seed_dir=out/f"seed-{seed}";seed_dir.mkdir(exist_ok=True)
        log=seed_dir/"audit.log"
        cmd=[args.binary,"--seed",str(seed),"--days",str(args.days),"--checkpoints",args.checkpoints,"--snapshot-directory",str(seed_dir/"snapshots")]
        print("+"," ".join(cmd),flush=True)
        with log.open("w",encoding="utf-8") as fh:
            p=subprocess.run(cmd,stdout=fh,stderr=subprocess.STDOUT,text=True)
        if p.returncode:return p.returncode
    return 0

def main()->int:
    ap=argparse.ArgumentParser()
    sub=ap.add_subparsers(dest="cmd",required=True)
    p=sub.add_parser("run");p.add_argument("--binary",required=True);p.add_argument("--seeds",default="4242001,874213954,1357911,2718281,3141592");p.add_argument("--days",type=int,default=1000);p.add_argument("--checkpoints",default="1,7,30,100,365,1000");p.add_argument("--out-dir",default="audit-results");p.set_defaults(func=run)
    p=sub.add_parser("report");p.add_argument("--logs",nargs="+",required=True);p.add_argument("--out-dir",default="audit-report");p.add_argument("--base-sha",default="");p.add_argument("--head-sha",default="");p.set_defaults(func=report)
    p=sub.add_parser("determinism");p.add_argument("--first",required=True);p.add_argument("--second",required=True);p.add_argument("--snapshot-a");p.add_argument("--snapshot-b");p.set_defaults(func=determinism)
    args=ap.parse_args();return args.func(args)
if __name__=="__main__":raise SystemExit(main())
