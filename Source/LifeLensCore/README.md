# LifeLensCore

Unreal Engine에 의존하지 않는 LifeLens 순수 C++17 시뮬레이션 코어입니다.

## Termux 검증

```bash
pkg install git clang cmake make
git clone https://github.com/sjLim91/lifelens-ue.git && cd lifelens-ue
git checkout task/02-core-sim
cmake -S Source/LifeLensCore -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
./build/ll_harness --days 1 --seed 42
```

옵션:

- `--days N`: 실행 일수
- `--seed S`: 결정론 WorldSeed
- `--tick-log`: 매 분 Needs 출력

같은 seed는 같은 이벤트 로그를 생성해야 합니다.
