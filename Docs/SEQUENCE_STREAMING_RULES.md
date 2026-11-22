# Sequence Streaming Rules (Corridor_DLv1)

## 목적
- `USequenceLevelStreamingComponent` + `USequenceStreamingConfigDataAsset`에서 사용하는 섹션별 로드/언로드 규칙을 고정한다.
- 기준 흐름: `A -> 01 -> B -> 02 -> C -> 03 -> D -> 04 -> E -> 05 -> F`

## 공통값 (모든 SectionRule)
- `bPlayerOnly = true`
- `bBlockOnLoad = false`
- `bBlockOnUnload = false`
- `UnloadDelay = 1.0`
- `EnterUnloadLevels = []`
- `ExitLoadLevels = []`
- `ExtraLoadLevels = []` (예외 라이트 사용 시만)
- `ExtraUnloadLevels = []` (예외 라이트 사용 시만)

## SectionId 목록
- `Corridor_A`
- `Corridor_Corner01`
- `Corridor_B`
- `Corridor_Corner02`
- `Corridor_C`
- `Corridor_Corner03`
- `Corridor_D`
- `Corridor_Corner04`
- `Corridor_E`
- `Corridor_Corner05`
- `Corridor_F`

## DA 입력표 (최종)
```yaml
Corridor_A:
  Priority: 10
  EnterLoadLevels:
    - LV_Theseus_Corridor_A_Mesh
    - LV_Theseus_Corridor_A_Light
    - LV_Theseus_Corridor_Corner01_Mesh
    - LV_Theseus_Corridor_Corner01_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_A_Mesh
    - LV_Theseus_Corridor_A_Light
    - LV_Theseus_Corridor_Corner01_Mesh
    - LV_Theseus_Corridor_Corner01_Light
  PreloadLevels:
    - LV_Theseus_Corridor_B_Mesh
    - LV_Theseus_Corridor_B_Light

Corridor_Corner01:
  Priority: 20
  EnterLoadLevels:
    - LV_Theseus_Corridor_A_Mesh
    - LV_Theseus_Corridor_A_Light
    - LV_Theseus_Corridor_Corner01_Mesh
    - LV_Theseus_Corridor_Corner01_Light
    - LV_Theseus_Corridor_B_Mesh
    - LV_Theseus_Corridor_B_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_Corner01_Mesh
    - LV_Theseus_Corridor_Corner01_Light
  PreloadLevels:
    - LV_Theseus_Corridor_Corner02_Mesh
    - LV_Theseus_Corridor_Corner02_Light

Corridor_B:
  Priority: 30
  EnterLoadLevels:
    - LV_Theseus_Corridor_Corner01_Mesh
    - LV_Theseus_Corridor_Corner01_Light
    - LV_Theseus_Corridor_B_Mesh
    - LV_Theseus_Corridor_B_Light
    - LV_Theseus_Corridor_Corner02_Mesh
    - LV_Theseus_Corridor_Corner02_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_B_Mesh
    - LV_Theseus_Corridor_B_Light
  PreloadLevels:
    - LV_Theseus_Corridor_C_Mesh
    - LV_Theseus_Corridor_C_Light

Corridor_Corner02:
  Priority: 40
  EnterLoadLevels:
    - LV_Theseus_Corridor_B_Mesh
    - LV_Theseus_Corridor_B_Light
    - LV_Theseus_Corridor_Corner02_Mesh
    - LV_Theseus_Corridor_Corner02_Light
    - LV_Theseus_Corridor_C_Mesh
    - LV_Theseus_Corridor_C_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_Corner02_Mesh
    - LV_Theseus_Corridor_Corner02_Light
  PreloadLevels:
    - LV_Theseus_Corridor_Corner03_Mesh
    - LV_Theseus_Corridor_Corner03_Light

Corridor_C:
  Priority: 50
  EnterLoadLevels:
    - LV_Theseus_Corridor_Corner02_Mesh
    - LV_Theseus_Corridor_Corner02_Light
    - LV_Theseus_Corridor_C_Mesh
    - LV_Theseus_Corridor_C_Light
    - LV_Theseus_Corridor_Corner03_Mesh
    - LV_Theseus_Corridor_Corner03_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_C_Mesh
    - LV_Theseus_Corridor_C_Light
  PreloadLevels:
    - LV_Theseus_Corridor_D_Mesh
    - LV_Theseus_Corridor_D_Light

Corridor_Corner03:
  Priority: 60
  EnterLoadLevels:
    - LV_Theseus_Corridor_C_Mesh
    - LV_Theseus_Corridor_C_Light
    - LV_Theseus_Corridor_Corner03_Mesh
    - LV_Theseus_Corridor_Corner03_Light
    - LV_Theseus_Corridor_D_Mesh
    - LV_Theseus_Corridor_D_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_Corner03_Mesh
    - LV_Theseus_Corridor_Corner03_Light
  PreloadLevels:
    - LV_Theseus_Corridor_Corner04_Mesh
    - LV_Theseus_Corridor_Corner04_Light

Corridor_D:
  Priority: 70
  EnterLoadLevels:
    - LV_Theseus_Corridor_Corner03_Mesh
    - LV_Theseus_Corridor_Corner03_Light
    - LV_Theseus_Corridor_D_Mesh
    - LV_Theseus_Corridor_D_Light
    - LV_Theseus_Corridor_Corner04_Mesh
    - LV_Theseus_Corridor_Corner04_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_D_Mesh
    - LV_Theseus_Corridor_D_Light
  PreloadLevels:
    - LV_Theseus_Corridor_E_Mesh
    - LV_Theseus_Corridor_E_Light

Corridor_Corner04:
  Priority: 80
  EnterLoadLevels:
    - LV_Theseus_Corridor_D_Mesh
    - LV_Theseus_Corridor_D_Light
    - LV_Theseus_Corridor_Corner04_Mesh
    - LV_Theseus_Corridor_Corner04_Light
    - LV_Theseus_Corridor_E_Mesh
    - LV_Theseus_Corridor_E_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_Corner04_Mesh
    - LV_Theseus_Corridor_Corner04_Light
  PreloadLevels:
    - LV_Theseus_Corridor_Corner05_Mesh
    - LV_Theseus_Corridor_Corner05_Light

Corridor_E:
  Priority: 90
  EnterLoadLevels:
    - LV_Theseus_Corridor_Corner04_Mesh
    - LV_Theseus_Corridor_Corner04_Light
    - LV_Theseus_Corridor_E_Mesh
    - LV_Theseus_Corridor_E_Light
    - LV_Theseus_Corridor_Corner05_Mesh
    - LV_Theseus_Corridor_Corner05_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_E_Mesh
    - LV_Theseus_Corridor_E_Light
  PreloadLevels:
    - LV_Theseus_Corridor_F_Mesh
    - LV_Theseus_Corridor_F_Light

Corridor_Corner05:
  Priority: 100
  EnterLoadLevels:
    - LV_Theseus_Corridor_E_Mesh
    - LV_Theseus_Corridor_E_Light
    - LV_Theseus_Corridor_Corner05_Mesh
    - LV_Theseus_Corridor_Corner05_Light
    - LV_Theseus_Corridor_F_Mesh
    - LV_Theseus_Corridor_F_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_Corner05_Mesh
    - LV_Theseus_Corridor_Corner05_Light
  PreloadLevels: []

Corridor_F:
  Priority: 110
  EnterLoadLevels:
    - LV_Theseus_Corridor_Corner05_Mesh
    - LV_Theseus_Corridor_Corner05_Light
    - LV_Theseus_Corridor_F_Mesh
    - LV_Theseus_Corridor_F_Light
  ExitUnloadLevels:
    - LV_Theseus_Corridor_F_Mesh
    - LV_Theseus_Corridor_F_Light
  PreloadLevels: []
```

## 예외 규칙 (건너 Corner Light)
- 특정 섹션에서 건너 코너 라이트만 보여야 할 때:
  - `ExtraLoadLevels += [LV_Theseus_Corridor_Corner0X_Light]`
  - `ExtraUnloadLevels += [LV_Theseus_Corridor_Corner0X_Light]`

## 테스트 메모
- 단일 섹션 테스트(`Corridor_A`만) 시에는 `PreloadLevels=[]`로 비우고 검증한다.
- `bEnableStreamingLog=true`로 `LoadRef++/--`와 `ActivateLevel 0 0 0`를 함께 확인한다.
