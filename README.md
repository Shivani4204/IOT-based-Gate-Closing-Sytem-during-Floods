# IoT Flood Mitigation Gate System 🌊🚧
### Major Project - PDEU ICT | Grade: 10/10 SGPA (Perfect Score)
**Status:** Field-tested prototype (dismantled after validation) | **Evidence:** [Certificate](docs/certificate/)

## ⚠️ Safety-Critical System Features
This project prioritizes **human life over property protection**:
- **Safety Interlock:** Bridge stays OPEN if humans detected during flood
- **Dual-confirmation:** IR sensors + Ultrasonic to prevent false positives
- **Hysteresis logic:** Prevents rapid open/close oscillation at threshold
- **Turbulence detection:** MPU6050 gyroscope monitors water flow violence

## 🎯 Engineering Highlights

### Multi-Sensor Fusion
| Sensor | Location | Purpose |
|--------|----------|---------|
| HC-SR04 (Bottom) | Under bridge | Water level detection (flood) |
| HC-SR04 (Top) | Above bridge | Human presence verification |
| IR Entry/Exit | Bridge ends | Directional human tracking |
| MPU6050 Gyroscope | Under bridge | Water flow turbulence detection |

### State Machine Logic

NORMAL → FLOOD_ALERT → [3-sec debounce]
↓                    ↓
EMERGENCY_OPEN ← Human detected?
(No human)          (Human present)
↓                    ↓
FLOOD_ACTIVE      Stay OPEN (safety)


## 🛡️ Safety Protocols
1. **Primary:** If `humanPresent = true` during flood → **Bridge stays OPEN**
2. **Secondary:** Ultrasonic top sensor confirms clearance before closing
3. **Fail-safe:** All sensors use pullups/timeout - system defaults to OPEN on sensor failure

## 🔧 Technical Implementation
- **Platform:** Arduino (C/C++)
- **Actuators:** Dual servo motors (redundancy for mechanical failure)
- **Logic:** Hysteresis band (10cm close / 13cm open) prevents rapid cycling
- **Safety Debounce:** 3-second confirmation before state change

## 📊 Validation
- **Field deployment:** [Add location] for [duration]
- **Academic evaluation:** Perfect score (10/10 SGPA)
- **Verification:** [Mentor name], HOD ICT Department

## 🎥 Evidence
- [Video Demonstration](docs/video_demo.mp4)
- [Academic Certificate](docs/certificate/)
- [Technical Report](reports/major_project_report.pdf) (if available)

## ⚠️ System Note
Prototype was dismantled after academic validation to harvest components for subsequent IoT projects. All code, documentation, and video evidence archived here.

---

**Built by:** Shivani Odedra | **Institution:** Pandit Deendayal Energy University | **Year:** 2025
