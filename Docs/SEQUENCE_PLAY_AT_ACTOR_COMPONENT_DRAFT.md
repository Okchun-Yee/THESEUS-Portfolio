# Sequence Play At Actor Component Draft

## 문서 목적
- 현재 레벨 블루프린트에서 직접 처리 중인 `플레이어/특정 액터 위치 기준 Level Sequence 재생` 로직을
  재사용 가능한 C++ 컴포넌트로 분리하기 위한 초안이다.
- 우선 적용 후보는 `Sequence27` 계열이며, 이후 다른 시퀀스에도 공통 사용 가능한 구조를 목표로 한다.

## 현재 로직 요약
- 현재 `BP_Theseus_Sequence_27_Trigger`에서는 대략 아래 순서로 Level Sequence를 재생한다.
1. `GetPlayerPawn(0)`로 플레이어 Pawn 획득
2. `IsValid` 확인
3. `TargetLS.SetBindingByTag("Player", [PlayerPawn], false)` 호출
4. `TargetLS.bOverrideInstanceData = true`
5. `TargetLS.DefaultInstanceData`를 `DefaultLevelSequenceInstanceData`로 캐스팅
6. `TransformOrigin.Location/Rotation`을 플레이어 기준으로 설정
7. `TargetLS.SequencePlayer.Play()`

- 즉, 현재 구조의 본질은:
  - 시퀀스의 바인딩 대상을 현재 액터(주로 플레이어)로 교체
  - 시퀀스 원점을 현재 액터 Transform 기준으로 덮어쓰기
  - 그 상태로 시퀀스 재생

## 컴포넌트화 목표
- 이 로직을 레벨 BP마다 반복 배선하지 않고,
  시퀀스를 재생할 액터에 `ActorComponent`를 부착해 공통 처리한다.
- 목표 책임:
  - 대상 `LevelSequenceActor` 참조 보관
  - 바인딩 태그 기준 재바인딩
  - Transform Origin 설정
  - 안전한 `Play()` 호출

## 권장 컴포넌트 형태
- 가칭:
  - `USequencePlayAtActorComponent`
- 권장 위치:
  - `Source/Dev_THESEUS/Public/Sequence/SequencePlayAtActorComponent.h`
  - `Source/Dev_THESEUS/Private/Sequence/SequencePlayAtActorComponent.cpp`

## 권장 사용 방식
- 컴포넌트는 `시퀀스를 재생시키고 싶은 액터`에 부착한다.
- 대표 대상:
  - 시퀀스 트리거 액터
  - 시퀀스 전용 래퍼 BP
  - 특정 이벤트 담당 액터

- 컴포넌트 세팅 예:
  - `TargetLevelSequenceActor`
  - `BindingTag = "Player"`
  - `AnchorMode = Owner | Player | ExplicitActor`
  - `LocationOffset`
  - `RotationOffset`
  - `bRebindByTag`
  - `bUseAnchorRotation`

## 권장 API
- 최소 API:
  - `bool PlayAtOwner()`
  - `bool PlayAtPlayer()`
  - `bool PlayAtActor(AActor* AnchorActor)`

- 내부 공통 함수 예:
  - `bool ResolveSequencePlayer(...)`
  - `bool ApplyBindingByTag(ALevelSequenceActor* InTargetLS, AActor* InActor)`
  - `bool ApplyTransformOrigin(ALevelSequenceActor* InTargetLS, const FTransform& InTransform)`
  - `bool PlayInternal(AActor* AnchorActor)`

## 동작 원칙
1. `TargetLevelSequenceActor` 유효성 검사
2. 앵커 액터 결정
  - `Owner`
  - `PlayerPawn`
  - 외부 인자 액터
3. 필요 시 `BindingTag`로 바인딩 재설정
4. `bOverrideInstanceData = true`
5. `DefaultInstanceData`를 `DefaultLevelSequenceInstanceData`로 확인
6. `TransformOrigin`에 앵커 Transform + 오프셋 반영
7. `SequencePlayer.Play()`

## 전제조건
- 모든 시퀀스에 무조건 적용되는 구조는 아니다.
- 아래 조건을 만족할 때 재사용성이 높다.
1. 시퀀스가 `BindingTag` 기반으로 대상을 교체할 수 있어야 한다.
2. 시퀀스가 `TransformOrigin` 기반 상대 재생을 허용해야 한다.
3. 절대 월드 좌표 의존이 강한 연출이면 컴포넌트 재사용성이 떨어진다.
4. `DefaultInstanceData`가 `DefaultLevelSequenceInstanceData` 경로로 유효해야 한다.

## 적용에 적합한 시퀀스
- 플레이어 위치 기준으로 재생되는 시퀀스
- 트리거 위치 기준으로 재생되는 짧은 연출
- 특정 액터를 `Player` 태그로 재바인딩해서 써야 하는 시퀀스

## 적용에 부적합하거나 주의가 필요한 시퀀스
- 레벨 내 고정 배치된 여러 액터를 절대좌표 기준으로 강하게 사용하는 시퀀스
- 시퀀스 내부 바인딩이 태그가 아니라 개별 레벨 액터 고정 참조에 의존하는 경우
- `DefaultInstanceData`가 기대 타입이 아니어서 캐스트 실패 가능성이 큰 경우

## 권장 로그
- 재사용 컴포넌트이므로 실패 원인 로그를 명확히 남긴다.
- 권장 로그 예:
  - `TargetLS is null`
  - `AnchorActor is null`
  - `BindingTag not found`
  - `DefaultInstanceData cast failed`
  - `SequencePlayer is null`
  - `PlayAtActor success`

## `Sequence27` 적용 시 체크포인트
1. 현재 `Player` 태그 바인딩을 실제로 사용 중인지 확인
2. `TransformOrigin`을 플레이어 위치로 옮겨도 기존 연출이 깨지지 않는지 확인
3. 현재 레벨 BP/트리거 BP에서 담당하던 시퀀스 재생 배선을 컴포넌트 호출로 치환 가능한지 확인
4. 캐스트 실패 시 현재 연출이 아예 재생되지 않는 구조인지 점검

## 예상 이점
- 레벨 블루프린트 반복 배선 제거
- 시퀀스 재생 규칙 일관화
- `Sequence27` 외 다른 시퀀스에도 동일 패턴 적용 가능
- 디버그/로그 위치 통일

## 현재 판단
- 이 구조는 컴포넌트로 분리하는 것이 맞다.
- 단, `시퀀스가 상대 재생 가능한 구조인지`가 재사용성의 핵심 조건이다.
- 따라서 구현 전 반드시 적용 대상 시퀀스를
  - `상대 재생 가능`
  - `절대 배치 의존`
  두 종류로 구분해야 한다.

## 다음 작업 후보
1. `USequencePlayAtActorComponent` C++ 골격 생성
2. `Sequence27`에 1차 적용
3. 기존 레벨 BP 재생 배선과 결과 비교
4. 성공 시 다른 시퀀스에 재사용 범위 확대

## 문서 상태
- 초안
- 실제 구현 전 `Sequence27` 기준으로 먼저 검증 필요

## 구현 반영 (2026-04-21)
- 실제 C++ 골격 추가 완료:
  - `Source/Dev_THESEUS/Public/Sequence/SequencePlayAtActorComponent.h`
  - `Source/Dev_THESEUS/Private/Sequence/SequencePlayAtActorComponent.cpp`
- 현재 반영된 핵심 API:
  - `PlayConfigured()`
  - `PlayAtOwner()`
  - `PlayAtPlayer()`
  - `PlayAtActor(AActor* AnchorActor)`
  - `StopPlayback()`
- 현재 반영된 핵심 책임:
  - `TargetLevelSequenceActor` 참조 보관
  - `BindingTag` 기준 런타임 재바인딩
  - `UDefaultLevelSequenceInstanceData` 기반 `TransformOrigin` 세팅
  - 앵커 기준 위치/회전 오프셋 적용 후 `SequencePlayer.Play()` 호출
- 남은 실제 적용 작업:
  1. `Sequence27` 또는 실제 레벨 BP 1곳에 부착
  2. 기존 `SetBindingByTag -> TransformOrigin -> Play` 배선을 컴포넌트 호출로 치환
  3. PIE 기준 상대 재생 결과와 카메라 첫 키 튐 여부 확인
