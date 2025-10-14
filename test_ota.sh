#!/bin/bash

# ESP32 OTA 測試腳本
# 用於測試 OTA 功能和創建發布版本

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

# 顏色輸出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 功能函數
print_header() {
    echo -e "${BLUE}=================================================${NC}"
    echo -e "${BLUE} $1${NC}"
    echo -e "${BLUE}=================================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# 檢查依賴
check_dependencies() {
    print_header "檢查依賴"
    
    if ! command -v pio &> /dev/null; then
        print_error "PlatformIO 未安裝或不在 PATH 中"
        echo "請安裝 PlatformIO: pip install platformio"
        exit 1
    fi
    print_success "PlatformIO 已安裝"
    
    if ! command -v git &> /dev/null; then
        print_error "Git 未安裝"
        exit 1
    fi
    print_success "Git 已安裝"
}

# 構建固件
build_firmware() {
    print_header "構建固件"
    
    echo "清理之前的構建..."
    pio run --target clean
    
    echo "構建 PNG 版本..."
    if pio run -e esp32-s3-devkitc-1; then
        print_success "PNG 版本構建成功"
    else
        print_error "PNG 版本構建失敗"
        exit 1
    fi
    
    echo "構建 BIN 版本..."
    if pio run -e esp32-s3-devkitc-1-bin; then
        print_success "BIN 版本構建成功"
    else
        print_error "BIN 版本構建失敗"
        exit 1
    fi
}

# 創建發布文件
create_release_files() {
    print_header "創建發布文件"
    
    # 創建發布目錄
    mkdir -p release
    
    # 複製固件文件
    if [ -f ".pio/build/esp32-s3-devkitc-1/firmware.bin" ]; then
        cp .pio/build/esp32-s3-devkitc-1/firmware.bin release/firmware-png.bin
        print_success "複製 PNG 固件文件"
    else
        print_error "PNG 固件文件不存在"
        exit 1
    fi
    
    if [ -f ".pio/build/esp32-s3-devkitc-1-bin/firmware.bin" ]; then
        cp .pio/build/esp32-s3-devkitc-1-bin/firmware.bin release/firmware-bin.bin
        print_success "複製 BIN 固件文件"
    else
        print_error "BIN 固件文件不存在"
        exit 1
    fi
    
    # 創建版本信息
    VERSION=${1:-"v1.0.0-test"}
    BUILD_DATE=$(date -u +%Y-%m-%dT%H:%M:%SZ)
    COMMIT=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
    
    cat > release/version.json << EOF
{
  "version": "$VERSION",
  "build_date": "$BUILD_DATE",
  "commit": "$COMMIT",
  "environments": {
    "png": "firmware-png.bin",
    "bin": "firmware-bin.bin"
  },
  "description": "ESP32 離線地圖 OTA 測試版本"
}
EOF
    
    print_success "創建版本信息文件: $VERSION"
    
    # 顯示文件大小
    echo ""
    echo "發布文件信息:"
    ls -lh release/
}

# 測試 HTTP 服務器
start_test_server() {
    print_header "啟動測試服務器"
    
    cd release
    
    # 檢查 Python 版本並啟動服務器
    if command -v python3 &> /dev/null; then
        PYTHON_CMD="python3"
    elif command -v python &> /dev/null; then
        PYTHON_CMD="python"
    else
        print_error "未找到 Python"
        exit 1
    fi
    
    print_warning "在 http://localhost:8000 啟動測試服務器"
    print_warning "版本檢查: http://localhost:8000/version.json"
    print_warning "固件下載: http://localhost:8000/firmware-png.bin"
    print_warning "按 Ctrl+C 停止服務器"
    
    $PYTHON_CMD -m http.server 8000
}

# 驗證 OTA 設置
verify_ota_config() {
    print_header "驗證 OTA 配置"
    
    echo "檢查 OTA 相關文件..."
    
    if [ -f "src/App/Utils/OTA/ota_updater.h" ]; then
        print_success "找到 OTA 頭文件"
    else
        print_error "OTA 頭文件缺失"
        exit 1
    fi
    
    if [ -f "src/App/Utils/OTA/ota_updater.cpp" ]; then
        print_success "找到 OTA 實現文件"
    else
        print_error "OTA 實現文件缺失"
        exit 1
    fi
    
    echo "檢查 platformio.ini 配置..."
    if grep -q "ENABLE_AUTO_OTA_CHECK" platformio.ini; then
        print_success "找到 OTA 配置"
    else
        print_warning "platformio.ini 中未找到 OTA 配置"
    fi
    
    echo "檢查主程序中的 OTA 代碼..."
    if grep -q "ota_updater.h" src/main.cpp; then
        print_success "主程序包含 OTA 代碼"
    else
        print_warning "主程序中未找到 OTA 代碼"
    fi
}

# 創建 Git 標籤和發布
create_git_release() {
    print_header "創建 Git 標籤"
    
    VERSION=${1:-"v1.0.0-test"}
    
    # 檢查是否有未提交的更改
    if ! git diff --quiet; then
        print_warning "有未提交的更改，是否繼續? (y/N)"
        read -r response
        if [[ ! "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
            print_error "已取消"
            exit 1
        fi
    fi
    
    # 創建標籤
    if git tag -l | grep -q "^$VERSION$"; then
        print_warning "標籤 $VERSION 已存在，是否刪除並重新創建? (y/N)"
        read -r response
        if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
            git tag -d "$VERSION"
            git push origin ":refs/tags/$VERSION" 2>/dev/null || true
        else
            print_error "已取消"
            exit 1
        fi
    fi
    
    git tag -a "$VERSION" -m "ESP32 離線地圖 OTA 測試版本 $VERSION"
    print_success "創建標籤 $VERSION"
    
    echo "推送標籤到遠程倉庫..."
    if git push origin "$VERSION"; then
        print_success "標籤已推送，GitHub Actions 將自動構建和發布"
        print_success "查看進度: https://github.com/$(git remote get-url origin | sed 's/.*github.com[:/]\([^.]*\).*/\1/')/actions"
    else
        print_error "推送標籤失敗"
        exit 1
    fi
}

# 顯示使用說明
show_usage() {
    echo "ESP32 OTA 測試腳本"
    echo ""
    echo "使用方法:"
    echo "  $0 [命令] [版本號]"
    echo ""
    echo "命令:"
    echo "  check       - 檢查依賴和 OTA 配置"
    echo "  build       - 構建固件"
    echo "  release     - 創建發布文件 (需要版本號)"
    echo "  server      - 啟動測試 HTTP 服務器"
    echo "  tag         - 創建 Git 標籤並推送 (需要版本號)"
    echo "  all         - 執行完整的測試流程 (需要版本號)"
    echo ""
    echo "範例:"
    echo "  $0 check"
    echo "  $0 build"
    echo "  $0 release v1.0.1"
    echo "  $0 tag v1.0.1"
    echo "  $0 all v1.0.1"
}

# 主函數
main() {
    case "${1:-}" in
        "check")
            check_dependencies
            verify_ota_config
            ;;
        "build")
            check_dependencies
            build_firmware
            ;;
        "release")
            if [ -z "${2:-}" ]; then
                print_error "請提供版本號"
                show_usage
                exit 1
            fi
            check_dependencies
            build_firmware
            create_release_files "$2"
            ;;
        "server")
            if [ ! -d "release" ] || [ ! -f "release/version.json" ]; then
                print_error "請先運行 'release' 命令創建發布文件"
                exit 1
            fi
            start_test_server
            ;;
        "tag")
            if [ -z "${2:-}" ]; then
                print_error "請提供版本號"
                show_usage
                exit 1
            fi
            create_git_release "$2"
            ;;
        "all")
            if [ -z "${2:-}" ]; then
                print_error "請提供版本號"
                show_usage
                exit 1
            fi
            check_dependencies
            verify_ota_config
            build_firmware
            create_release_files "$2"
            create_git_release "$2"
            ;;
        *)
            show_usage
            exit 1
            ;;
    esac
}

# 運行主函數
main "$@"