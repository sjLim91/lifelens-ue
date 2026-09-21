# LifeLens Web Client

LifeLens의 정식 browser/PWA observer client입니다.

## 원칙

- simulation truth는 `Source/LifeLensCore` 하나만 사용합니다.
- 웹이 별도의 World/Character simulation을 만들지 않습니다.
- WASM이 없으면 UI는 fail-closed하고 가짜 resident/terrain을 표시하지 않습니다.
- WorldSeed와 64-bit entity IDs는 JavaScript Number로 강제 변환하지 않고 문자열로 전달합니다.
- 현재 production WorldGenerationVersion은 Core의 현재 버전을 따르며, World v2 v3 switch 전에는 임의로 올리지 않습니다.

## 로컬 빌드

Emscripten SDK가 PATH에 있는 환경:

```bash
python Tools/build_web_client.py
python Tools/serve_web_client.py
```

그 후 출력된 localhost 주소를 브라우저에서 엽니다.

`build_web_client.py`는 생성된 `lifelens_core.js/.wasm`만
`Clients/Web/generated/`에 staging합니다. 생성물은 Git에 commit하지 않습니다.

## 현재 milestone

- PWA shell.
- LifeLensCore WebClientBridge.
- Emscripten/Embind adapter.
- actual Core WorldSeed / resident observation / terrain + hydrology truth preview.
- no fake simulation fallback.

다음 visual milestone:
- WebGPU terrain mesh.
- water surface.
- biome coverage / instanced vegetation.
- glTF resident presentation.
- animation mapping.
