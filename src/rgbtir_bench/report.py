from __future__ import annotations

import csv
import html
import json
from pathlib import Path
from statistics import mean


def summarize(rows: list[dict]) -> list[dict]:
    groups: dict[str, list[dict]] = {}
    for row in rows:
        groups.setdefault(row["method"], []).append(row)
    metrics = ["matches", "correct_matches", "precision_px", "mean_error_px", "median_error_px", "p95_error_px", "mma_1px", "mma_3px", "mma_5px", "mma_10px", "inlier_ratio", "homography_rmse_px", "coverage_4x4", "runtime_ms"]
    summary = []
    for method, items in groups.items():
        result = {"method": method, "pairs": len(items), "failures": sum(bool(x.get("error")) for x in items)}
        for metric in metrics:
            values = [float(x[metric]) for x in items if x.get(metric) is not None and str(x.get(metric)).lower() != "nan"]
            result[metric] = mean(values) if values else None
        summary.append(result)
    return sorted(summary, key=lambda x: x["method"])


def write_reports(output_dir: Path, config: dict, rows: list[dict]) -> list[dict]:
    output_dir.mkdir(parents=True, exist_ok=True)
    summary = summarize(rows)
    _write_csv(output_dir / "per_pair.csv", rows)
    _write_csv(output_dir / "summary.csv", summary)
    (output_dir / "results.json").write_text(json.dumps({"config": config, "summary": summary, "pairs": rows}, indent=2, ensure_ascii=False, allow_nan=True), encoding="utf-8")
    (output_dir / "report.html").write_text(_html_report(config, summary), encoding="utf-8")
    return summary


def _write_csv(path: Path, rows: list[dict]) -> None:
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    fields = list(dict.fromkeys(k for row in rows for k in row))
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def _html_report(config: dict, rows: list[dict]) -> str:
    heads = ["Method", "Pairs", "Precision", "MMA@3", "Inlier ratio", "Median error (px)", "Runtime (ms)"]
    body = []
    for row in rows:
        vals = [row["method"], row["pairs"], _pct(row["precision_px"]), _pct(row["mma_3px"]), _pct(row["inlier_ratio"]), _num(row["median_error_px"]), _num(row["runtime_ms"])]
        body.append("<tr>" + "".join(f"<td>{html.escape(str(v))}</td>" for v in vals) + "</tr>")
    return f"""<!doctype html><html lang='vi'><meta charset='utf-8'><title>RGB–TIR Match Bench</title>
<style>body{{font:15px system-ui;max-width:1100px;margin:40px auto;padding:0 20px;color:#172033}}h1{{color:#4f46e5}}table{{border-collapse:collapse;width:100%}}th,td{{border-bottom:1px solid #ddd;padding:10px;text-align:right}}th:first-child,td:first-child{{text-align:left}}code{{background:#eef2ff;padding:3px 6px}}</style>
<h1>RGB–TIR Match Bench</h1><p>Đánh giá ghép điểm trên dữ liệu RGB–IR đã căn chỉnh. Ngưỡng đúng: <code>{config['correctness_px']} px</code>.</p>
<table><thead><tr>{''.join(f'<th>{x}</th>' for x in heads)}</tr></thead><tbody>{''.join(body)}</tbody></table>
<p><small>Không diễn giải các metric 2D này thành RMSE 3D của bài báo.</small></p></html>"""


def _pct(value) -> str:
    return "—" if value is None else f"{100 * value:.2f}%"


def _num(value) -> str:
    return "—" if value is None else f"{value:.2f}"

