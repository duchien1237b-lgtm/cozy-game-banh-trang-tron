# Hiệu ứng thao tác nấu ăn

`cooking_fx.png`: atlas PNG RGBA tạo bằng công cụ imagegen tích hợp ngày 2026-10-30, giữ nguyên alpha từ ảnh tạo ra. Đồng bộ tại `MaNguon/assets/images/cozy/effects/` và assets của bản Release.

Atlas 3 cột × 2 hàng, đọc từ trái sang phải: mảnh bánh, vệt xé, tia sáng, Miss, Good, Perfect. `ui/CookingEffects` cắt vùng hiển thị theo alpha từng ô lúc tải; dùng lại texture và chỉ cập nhật vị trí, xoay, kích thước, độ mờ khi chạy. Tối đa 96 sprite đang sống.

- Xé: chỉ phát sinh vụn khi thời gian xé thực sự tăng do rê chuột. Mỗi giai đoạn hoàn thành có thêm vụn tỏa ra; tự rơi và mờ dần.
- Topping: thanh lực xanh → vàng → cam, hiển thị 1–2–3; con trỏ sáng và nháy khi đạt mức mới. Thả hợp lệ có tia sáng theo lượng thực sự thêm; từng icon chạm tô có ánh sáng nhỏ.
- Trộn: thanh có viền vàng, vùng xanh/Perfect rõ ràng, vạch chạy có đuôi sáng. Vụ nổ Miss/Good/Perfect xuất hiện tại vị trí vừa bấm và tự biến mất, kể cả nhịp cuối chuyển sang giao món.

## Prompt cuối cùng — built-in imagegen

Use case: stylized-concept. Asset type: a production-ready transparent sprite atlas for a cozy Vietnamese rice-paper cooking game, warm hand-painted cartoon style with fine caramel outlines, creamy highlights, green jade and honey gold palette. Create ONE square 1024x1024 image with precisely 3 columns by 2 rows of equal cells, no drawn grid. Each sprite centered in its cell, with large transparent padding and no overlap into another cell. Top left: a single irregular torn ivory rice-paper shard with delicate woven rice-paper texture. Top middle: a small curved ivory tearing slash with three tiny crumbs, compact horizontal arc. Top right: a golden four-point glint with subtle amber halo, isolated. Bottom left: a small soft coral impact puff for missed timing, 3 rounded lobes and short coral motion ticks, transparent center. Bottom middle: jade green circular swoosh with three tiny cream sparkles for a good timing hit, transparent center. Bottom right: celebratory honey-gold starburst ring with tiny golden stars for perfect timing, transparent center. All six are individual VFX sprites viewed straight on, readable at 24-80px size. No text, labels, frames, background, checkerboard, scenery, UI mockup, or extra objects. Actual alpha transparency.
