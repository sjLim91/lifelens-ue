# LifeLens Android Fast Pipeline

## 목적

Android 검증 때마다 Unreal Engine 전체 C++를 다시 컴파일하지 않는다.

`android-apk.yml`은 세 가지 수동 모드를 제공한다.

- `seed`: UE 5.6 정확한 revision + NDK r25b 조합의 Android 엔진 컴파일 산출물을 1회 생성한다. Linux cook 도구도 함께 준비한다. 생성된 재사용 번들은 암호화한 뒤 GitHub Actions cache에 저장한다.
- `fast`: seed cache가 반드시 있어야 한다. 캐시가 없으면 즉시 실패하며 full build로 자동 fallback하지 않는다. 캐시를 복구한 뒤 `-NoEngineChanges`로 LifeLens 프로젝트 변경분만 컴파일하고 APK를 패키징한다.
- `full`: 문제 진단/새 UE revision/새 toolchain에서만 사용하는 느린 fallback이다.

## 보안 / 라이선스 경계

저장소는 public이므로 Unreal Engine 컴파일 산출물을 평문 Release/Artifact로 배포하지 않는다.

재사용 캐시는 `EPIC_GHCR_PAT`에서 런타임에 제공되는 비밀값으로 AES-256-CBC/PBKDF2 암호화하여 저장한다. 캐시에 secret 자체는 저장하지 않는다. Epic 접근 secret이 없는 untrusted run은 캐시 내용을 복호화할 수 없다.

## 캐시 무효화

캐시 key에는 Epic `5.6` 브랜치의 정확한 commit SHA와 NDK 계열이 포함된다.

따라서 UE 5.6 branch head가 바뀌면 기존 cache를 잘못 재사용하지 않고 새 seed가 필요하다.

현재 key 형식:

`lifelens-ue56-android-r25b-<UE_SHA>-v1`

## 기본 실행 순서

1. 새 엔진 revision 또는 최초 1회: `mode=seed`
2. 이후 일반 APK 확인: `mode=fast`
3. fast가 `FAST_ENGINE_CACHE_MISS`를 반환하면 seed를 1회 실행한다.
4. fast에서 engine 변경 필요 오류가 발생하면 cache 범위를 확인한다. 자동 full rebuild는 하지 않는다.
5. `full`은 seed/cache 문제 진단 때만 수동 사용한다.

## 성공 판정

- Compile 성공과 APK 성공을 구분한다.
- 실제 `.apk`가 생성되고 Artifact/Release 업로드까지 끝나기 전에는 APK 성공으로 보지 않는다.
- fast mode에서 Engine 전체 재컴파일이 시작되면 설계 위반이다. `-NoEngineChanges`가 이를 차단해야 한다.

## 비용 원칙

표준 public GitHub-hosted runner와 기본 Actions cache 범위만 사용한다. 캐시 암호화 파일이 9 GB를 초과하면 저장을 중단하고 범위를 더 줄인다. 유료 cache 확대를 자동으로 사용하지 않는다.
