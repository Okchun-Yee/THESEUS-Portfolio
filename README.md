# THESEUS — Portfolio

> 1인칭 리니어 호러 슈터 · Unreal Engine 5.7 · 3인 팀(기획 / 아트 / 개발) 중 **개발 담당**
> 개발 기간 2026.02 – 2026.06 · **IndieCraft 2026 챌린저 부문 최우수상**

이 저장소는 THESEUS 프로젝트에서 **제가 직접 작성한 C++ 게임플레이 코드**와 설계 문서를 정리한 포트폴리오용 저장소입니다.
원본 프로젝트는 유료 에셋을 포함하고 있어 비공개로 유지하며, 이 저장소에는 에셋·맵·블루프린트(`.uasset`) 파일이 포함되어 있지 않습니다.
따라서 이 저장소의 코드는 단독으로 빌드·실행되지 않습니다.

---

## 플레이 영상

<!-- TODO: 플레이 영상 링크 추가 -->

---

## 아키텍처: Blueprint + C++ ActorComponent 하이브리드

| 레이어 | 담당 |
|---|---|
| **Blueprint** | 시네마틱·시퀀스 흐름, 레벨 단위 연출 조립 |
| **C++ ActorComponent / Subsystem** | 기능과 상태 — 스트리밍, 무기 상태, AI 스폰, 이벤트 전달 |

초기에는 Blueprint로 시작했고, 상태와 기능 로직이 커지면서 핵심 구현을 C++ 컴포넌트로 옮겼습니다.
Blueprint를 버린 것이 아니라 **"기능은 C++ 컴포넌트, 조립은 Blueprint"** 로 역할을 나눈 구조입니다.

- `Common/` 폴더는 다른 도메인과 양방향 참조가 없습니다. 범용 컴포넌트끼리 서로 알지 못하고, 조합은 Blueprint에서 이루어집니다.
- `AI → Combat → Sequence → AI` 사이에는 `.cpp` 수준의 순환 참조가 있지만 헤더 수준의 컴파일 의존은 없습니다. 짧은 개발 일정 안에서 인지하고 수용한 트레이드오프입니다.

규모: C++ 클래스 28개, 약 9,900줄

---

## 대표 시스템

### 1. 데이터 기반 레벨 스트리밍 — `SequenceLevelStreamingComponent`
- 섹션 진입/이탈 시 어떤 서브레벨을 로드·언로드할지 `DataAsset`(`SequenceStreamingConfigDataAsset`)의 섹션 규칙으로 정의합니다.
- 레벨별 **참조 카운트**(`LevelRefCountMap`)로 여러 섹션이 같은 레벨을 공유해도 안전하게 관리하고, 카운트가 0이 될 때만 지연 언로드합니다. 재진입 시 대기 중인 언로드는 취소합니다.
- `bEnableStreamingLog`로 `LoadRef++ / LoadRef--` 로그를 남겨, 카운트가 0이 될 때 언로드되는 동작을 PIE 로그로 검증했습니다.
- **결과:** 전체 레벨을 항상 로드하던 빌드는 30fps, 스트리밍 적용 후 같은 조건에서 **80–120fps**. (30fps는 프레임 제한이 아닌 실측값)
- 설계 문서: [`Docs/SEQUENCE_STREAMING_RULES.md`](Docs/SEQUENCE_STREAMING_RULES.md)

### 2. 무기 JAM 상태 전파 — `WeaponJamComponent`
- 발사 횟수를 세다가 임계값에 도달하면 총기가 걸리는(JAM) 연출용 상태 컴포넌트입니다.
- 상태 변화는 델리게이트(`OnJamStateChanged` 등)로 Blueprint와 AI 쪽에 전파합니다.
- 이미 JAM 상태일 때 들어오는 발사 등록은 무시하고 로그로 남겨, 중복 처리 방어가 실제로 동작하는지 확인할 수 있게 했습니다.

### 3. 스폰 포인트 런타임 설정 주입 — `AISpawnComponent`
- 베이스 에셋의 기본 AI 생성 위에 **설정 주입 계층**을 만들었습니다.
- 스폰 설정값은 4단계 우선순위로 결정됩니다.
  1. `SpawnPointArrayBySpawnIndex` — 유닛별 설정
  2. `SpawnPointArrayByPointIndex` — 포인트 슬롯별 설정
  3. `XXXOverrides[PointIndex]` — 컴포넌트 오버라이드 배열
  4. `bDefaultXXX` — 컴포넌트 기본값
- 어느 단계에서 값이 결정됐는지 `ResolvedSource`로 로그에 남겨, 레벨 디자이너가 설정 결과를 추적할 수 있게 했습니다.

### 4. 이벤트 버스 — `TheseusEventBusSubsystem`
- `GameInstanceSubsystem` 기반 이벤트 버스로, 티커 테이프(종이 인쇄) 연출 요청과 조준 강제 해제 신호를 발행·구독합니다.
- 발신자(`TickerPaperManager`)와 수신자(플레이어 캐릭터 등)가 서로를 직접 참조하지 않도록 분리했습니다.

### 공통 설계 원칙
방어 로직은 "있다"고 주장하는 데서 끝내지 않고 **관찰 가능하게** 만들었습니다.
스트리밍의 `LoadRef` 로그, JAM 중복 방어 로그, 스폰 설정의 `ResolvedSource` 로그가 모두 같은 원칙에서 나왔습니다.

---

## 폴더 구조

```
Source/Dev_THESEUS/
├── AI/        스폰, 스플라인 이동, 엿보기 순찰, BT 서비스
├── Combat/    무기 JAM, 트레이서
├── Sequence/  레벨 스트리밍, 컷신 체인, 분기 선택, 이벤트 버스, 티커 페이퍼
└── Common/    범용 트리거(LookAt, 조명 깜빡임/파괴), 스플라인 이동
Docs/          설계 문서
```

---

## 설계 문서

| 문서 | 내용 |
|---|---|
| [SEQUENCE_STREAMING_RULES](Docs/SEQUENCE_STREAMING_RULES.md) | 복도 구간 섹션별 로드/언로드 규칙 |
| [SEQUENCE27_APPROACH_DRAFT](Docs/SEQUENCE27_APPROACH_DRAFT.md) | 다수 AI가 좁은 복도에서 한 점으로 수렴하는 병목 문제 분석과 접근 구조 후보 비교 |
| [SEQUENCE_PLAY_AT_ACTOR_COMPONENT_DRAFT](Docs/SEQUENCE_PLAY_AT_ACTOR_COMPONENT_DRAFT.md) | 레벨 블루프린트에 반복되던 시퀀스 재생 로직을 C++ 컴포넌트로 분리한 설계 |

---

## 베이스 에셋과 기여 범위

- FPS 기본 기능(캐릭터, 무기, 기본 AI)은 Fab의 FPS 에셋을 베이스로 사용했습니다: <https://www.fab.com/listings/90ba076a-dc9a-4782-9ac8-dc2ed4f06405>
- 이 저장소의 C++ 코드는 에셋 위에 새로 추가한 기능이며, 에셋의 원본 코드나 파일은 포함하지 않습니다.
- 기획·아트는 팀원이 담당했습니다.

---

Copyright 2026, Okchun-Yee. 코드는 포트폴리오 열람 목적으로 공개합니다.
