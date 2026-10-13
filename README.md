# RGB–TIR Match Bench (Qt)

Ứng dụng desktop Qt để kiểm chứng khả năng ghép đặc trưng cục bộ giữa ảnh RGB và ảnh nhiệt trên **LLVIP** và **RoadScene**. Dự án được xây dựng theo hướng thực nghiệm của bài báo:

> L. Morelli et al., “Evaluating Deep Learning Local Features for RGB-Thermal Image Matching and 3D InfraRed Thermography,” *Geomatics*, 6(4), 83, 2026. DOI: [10.3390/geomatics6040083](https://doi.org/10.3390/geomatics6040083).

Điểm cốt lõi được kiểm chứng là giả thuyết: mô hình ghép ảnh học sâu đã huấn luyện trên ảnh đơn phương thức vẫn có thể tổng quát hóa sang RGB–TIR mà không cần huấn luyện lại.

![Giao diện RGB–TIR Match Bench](docs/gui.png)

## Chức năng

- Giao diện **PySide6 / Qt 6**, hỗ trợ kéo‑thả thư mục ảnh.
- Tự nhận dạng cấu trúc LLVIP và RoadScene.
- Ghép ảnh theo tên tệp; có cảnh báo rõ ràng nếu phải ghép theo thứ tự.
- Benchmark nhiều phương pháp trong một lần chạy:
  - SIFT + nearest-neighbor ratio test;
  - ORB + nearest-neighbor ratio test;
  - SuperPoint + LightGlue;
  - DISK + LightGlue;
  - ALIKED + LightGlue;
  - LoFTR.
- Chạy nền bằng `QThread`, giao diện không bị treo; có tiến độ và nút dừng.
- Xuất `summary.csv`, `per_pair.csv`, `results.json` và `report.html`.
- Chạy bằng GUI hoặc dòng lệnh; có test tự động và GitHub Actions.

## Phạm vi kiểm chứng và giới hạn khoa học

Bài báo đánh giá đầy đủ cả ghép điểm đa phổ, bundle adjustment và sai số 3D trên các check point được đo trắc địa ở mức milimet. LLVIP và RoadScene chỉ cung cấp các cặp RGB–IR đã căn chỉnh, không cung cấp đầy đủ nội suy camera, pose, tie tracks đa góc nhìn và check point 3D tương đương Thermitage.

Vì vậy dự án này đánh giá **đúng phần image matching 2D** của bài báo, không tuyên bố tái tạo nguyên xi RMSE 3D. Để tái lập phần 3D, cần dùng [Thermitage](https://github.com/3DOM-FBK/Thermitage) và pipeline SfM/bundle adjustment (ví dụ Deep-Image-Matching + COLMAP), cùng các check point khảo sát.

## Metric

Với cặp ảnh đã căn chỉnh, tọa độ đúng lý tưởng là cùng vị trí sau khi chuẩn hóa độ phân giải. Với mỗi match `(xᵢᴿᴳᴮ, xᵢᴵᴿ)`, sai số:

```text
eᵢ = ||xᵢᴿᴳᴮ − scale(xᵢᴵᴿ)||₂
```

- `Precision@τ`: tỷ lệ match có `eᵢ ≤ τ` (mặc định `τ = 5 px`).
- `MMA@1/3/5/10`: Mean Matching Accuracy tại các ngưỡng pixel cố định.
- `Mean / median / P95 error`: phân phối sai số căn chỉnh.
- `Inlier ratio`: tỷ lệ inlier của homography RANSAC.
- `Homography RMSE`: sai số của các inlier sau ước lượng homography.
- `Coverage 4×4`: tỷ lệ ô không gian có match, tránh trường hợp match chỉ tập trung ở một vùng.
- `Runtime`: thời gian suy luận mỗi cặp ảnh.

Lưu ý: độ chính xác của kết quả phụ thuộc trực tiếp vào chất lượng registration gốc. Với LLVIP, một số cặp vẫn có thể còn lệch cục bộ; có thể tăng `τ` hoặc lọc riêng các cặp đó, nhưng phải giữ cùng cấu hình khi so sánh các phương pháp.

## Yêu cầu

- Python 3.10–3.12 (khuyến nghị 3.11).
- macOS, Linux hoặc Windows.
- SIFT/ORB: CPU là đủ.
- Các mô hình học sâu: PyTorch; NVIDIA CUDA được ưu tiên, Apple Silicon có thể dùng MPS. Lần chạy đầu cần Internet để tải trọng số chính thức.

## Cài đặt

### Bản đầy đủ

```bash
git clone https://github.com/soncris3012/rgbtir-match-bench-qt.git
cd rgbtir-match-bench-qt
python3.11 -m venv .venv
source .venv/bin/activate              # Windows: .venv\Scripts\activate
python -m pip install --upgrade pip
pip install -r requirements-deep.txt
```

### Chỉ SIFT/ORB, không cần PyTorch

```bash
pip install -r requirements.txt
```

## Chuẩn bị dữ liệu

Không đưa dữ liệu vào Git vì kích thước lớn và điều khoản cấp phép. Hãy tải từ nguồn chính thức:

- [LLVIP chính thức](https://github.com/bupt-ai-cz/LLVIP) — xem kỹ `Term of Use and License.md`; dữ liệu được công bố cho mục đích phi thương mại theo điều khoản của tác giả.
- [RoadScene chính thức](https://github.com/hanna-xu/RoadScene) — 221 cặp visible/IR đã căn chỉnh.

Cấu trúc LLVIP được hỗ trợ:

```text
LLVIP/
├── visible/
│   ├── train/*.jpg
│   └── test/*.jpg
└── infrared/
    ├── train/*.jpg
    └── test/*.jpg
```

Cấu trúc RoadScene được hỗ trợ:

```text
RoadScene/
├── crop_LR_visible/*
└── cropinfrared/*
```

Kiểm tra nhanh trước khi chạy:

```bash
python scripts/check_dataset.py /duong/dan/LLVIP
```

## Chạy giao diện Qt

```bash
python -m rgbtir_bench
# hoặc sau khi cài package:
rgbtir-bench
```

Quy trình:

1. Bấm **Nhận dạng thư mục gốc** hoặc kéo riêng hai thư mục `visible` và `infrared` vào hai vùng thả.
2. Chọn một hoặc nhiều matcher.
3. Chọn số cặp tối đa, ngưỡng pixel, số keypoint và thiết bị.
4. Bấm **Chạy benchmark**.
5. Xem bảng tổng hợp và mở báo cáo HTML. Mỗi lần chạy được lưu trong `runs/YYYYMMDD-HHMMSS/`.

Để so sánh công bằng, chương trình lấy cùng một mẫu cặp ảnh cho mọi matcher với seed cố định `42`.

## Chạy dòng lệnh

```bash
rgbtir-bench \
  --visible /data/LLVIP/visible/test \
  --thermal /data/LLVIP/infrared/test \
  --methods SIFT "SuperPoint + LightGlue" "ALIKED + LightGlue" LoFTR \
  --max-pairs 500 \
  --threshold 5 \
  --device auto \
  --output runs
```

Ví dụ RoadScene:

```bash
rgbtir-bench \
  --visible /data/RoadScene/crop_LR_visible \
  --thermal /data/RoadScene/cropinfrared \
  --methods SIFT "SuperPoint + LightGlue" \
  --max-pairs 221
```

## Đọc kết quả

```text
runs/20261013-103000/
├── per_pair.csv   # metric của từng cặp và từng matcher
├── summary.csv    # trung bình theo matcher
├── results.json   # cấu hình + kết quả máy đọc được
└── report.html    # báo cáo mở trực tiếp trong trình duyệt
```

Nên báo cáo ít nhất `Precision@5`, `MMA@3`, median error, inlier ratio, coverage và thời gian. Không chỉ chọn matcher có nhiều match nhất: số match lớn nhưng precision/coverage thấp có thể không đủ tin cậy cho SfM.

## Kiểm thử

```bash
pip install -e '.[test]'
pytest
```

Test hiện có kiểm tra ghép cặp LLVIP, fallback theo thứ tự, metric identity và chuẩn hóa khi hai ảnh khác độ phân giải.

## Xử lý lỗi thường gặp

- **`Thiếu backend LightGlue/LoFTR`**: cài `requirements-deep.txt` trong Python 3.10–3.12.
- **CUDA hết bộ nhớ**: giảm `Keypoint tối đa`, giảm số cặp, hoặc chọn `cpu`.
- **MPS có toán tử chưa hỗ trợ**: chọn `cpu`; kết quả metric không đổi, chỉ chậm hơn.
- **Không ghép được tên tệp**: kiểm tra hai modality có cùng basename. Fallback theo thứ tự chỉ an toàn khi thứ tự hai thư mục thực sự tương ứng.
- **Precision thấp bất thường**: mở trực quan dữ liệu nguồn để kiểm tra residual misalignment; không điều chỉnh ngưỡng riêng cho từng matcher.
- **Lần chạy đầu chậm**: các backend học sâu đang tải pretrained weights.

## Liên hệ với bài báo

Các backend SuperPoint, DISK, ALIKED, LightGlue và LoFTR bám sát nhóm thuật toán được bài báo thảo luận/thử nghiệm; SIFT là baseline thủ công đơn phương thức. Bài báo cũng khảo sát DeDoDe, SuperGlue, KeyNet+HardNet, RIFT2 và SRIF. Chúng chưa được bật mặc định ở bản đầu này vì API/trọng số phân tán hoặc phụ thuộc MATLAB; kiến trúc `matchers/registry.py` cho phép bổ sung backend mới mà không đổi GUI hay metric.

## Trích dẫn dữ liệu

Nếu dùng kết quả trong công bố, hãy trích dẫn bài báo gốc, LLVIP và RoadScene/FusionDN theo hướng dẫn trong repository chính thức của từng bộ dữ liệu.

## Giấy phép

Mã nguồn dự án dùng giấy phép MIT. Trọng số mô hình và dữ liệu **không** thuộc giấy phép MIT này; chúng tuân theo giấy phép riêng của từng tác giả/nhà cung cấp.

