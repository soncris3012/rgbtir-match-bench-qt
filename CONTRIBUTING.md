# Contributing

1. Tạo nhánh ngắn cho thay đổi.
2. Giữ interface matcher trả về `MatchResult(points0, points1, confidence)`.
3. Thêm backend vào `matchers/registry.py` và thêm test không phụ thuộc tải trọng số nếu có thể.
4. Chạy `pytest` trước khi mở pull request.
5. Không commit dữ liệu LLVIP/RoadScene, pretrained weights hoặc nội dung có giấy phép không tương thích.

