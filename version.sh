#!/bin/bash

# 版本管理腳本 - 用於更新項目中的版本號

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

# 顏色輸出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# 獲取當前版本
get_current_version() {
    if [ -f "src/App/Version.h" ]; then
        # 尋找 VERSION_SOFTWARE 定義
        if grep -q "VERSION_SOFTWARE" src/App/Version.h; then
            version=$(grep "VERSION_SOFTWARE" src/App/Version.h | cut -d'"' -f2)
            # 去除 v 前綴（如果有的話）
            echo "${version#v}"
        elif grep -q "APP_VERSION" src/App/Version.h; then
            version=$(grep "APP_VERSION" src/App/Version.h | cut -d'"' -f2)
            echo "${version#v}"
        else
            echo "1.0.0"
        fi
    elif grep -q "String currentVersion" src/main.cpp; then
        version=$(grep "String currentVersion" src/main.cpp | cut -d'"' -f2)
        echo "${version#v}"
    else
        echo "1.0.0"
    fi
}

# 更新版本號
update_version() {
    local new_version="$1"
    
    # 更新 Version.h（如果存在）
    if [ -f "src/App/Version.h" ]; then
        # 更新 VERSION_SOFTWARE 或 APP_VERSION
        if grep -q "VERSION_SOFTWARE" src/App/Version.h; then
            sed -i "s/#define VERSION_SOFTWARE[[:space:]]*\".*\"/#define VERSION_SOFTWARE        \"v$new_version\"/" src/App/Version.h
            print_success "更新 Version.h (VERSION_SOFTWARE)"
        elif grep -q "APP_VERSION" src/App/Version.h; then
            sed -i "s/#define APP_VERSION[[:space:]]*\".*\"/#define APP_VERSION \"v$new_version\"/" src/App/Version.h
            print_success "更新 Version.h (APP_VERSION)"
        fi
    fi
    
    # 更新 main.cpp 中的版本號
    if grep -q "String currentVersion" src/main.cpp; then
        sed -i "s/String currentVersion = \".*\"/String currentVersion = \"$new_version\"/" src/main.cpp
        print_success "更新 main.cpp"
    fi
    
    # 創建或更新 Version.h（如果不存在）
    if [ ! -f "src/App/Version.h" ]; then
        cat > src/App/Version.h << EOF
#ifndef VERSION_H
#define VERSION_H

#define APP_VERSION "$new_version"
#define APP_NAME "ESP32 OffLine Map"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

#endif // VERSION_H
EOF
        print_success "創建 Version.h"
    fi
}

# 顯示當前版本
show_current_version() {
    local current=$(get_current_version)
    echo "當前版本: $current"
}

# 增加版本號
increment_version() {
    local current=$(get_current_version)
    local type="$1"
    
    # 解析版本號 (假設格式為 x.y.z)
    local major=$(echo "$current" | cut -d'.' -f1)
    local minor=$(echo "$current" | cut -d'.' -f2)
    local patch=$(echo "$current" | cut -d'.' -f3)
    
    case "$type" in
        "major")
            major=$((major + 1))
            minor=0
            patch=0
            ;;
        "minor")
            minor=$((minor + 1))
            patch=0
            ;;
        "patch")
            patch=$((patch + 1))
            ;;
        *)
            print_error "未知的版本類型: $type"
            echo "請使用: major, minor, 或 patch"
            exit 1
            ;;
    esac
    
    local new_version="$major.$minor.$patch"
    echo "版本號從 $current 更新到 $new_version"
    update_version "$new_version"
    echo "$new_version"
}

# 驗證版本格式
validate_version() {
    local version="$1"
    # 使用 grep 來驗證版本格式 (x.y.z)
    if ! echo "$version" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
        print_error "無效的版本格式: $version"
        echo "版本號格式應為: x.y.z (例如: 1.2.3)"
        exit 1
    fi
}

# 顯示使用說明
show_usage() {
    echo "版本管理腳本"
    echo ""
    echo "使用方法:"
    echo "  $0 current                    - 顯示當前版本"
    echo "  $0 set <version>              - 設置特定版本號"
    echo "  $0 increment <major|minor|patch> - 增加版本號"
    echo ""
    echo "範例:"
    echo "  $0 current"
    echo "  $0 set 1.2.3"
    echo "  $0 increment patch"
    echo "  $0 increment minor"
    echo "  $0 increment major"
}

# 主函數
main() {
    case "${1:-}" in
        "current")
            show_current_version
            ;;
        "set")
            if [ -z "${2:-}" ]; then
                print_error "請提供版本號"
                show_usage
                exit 1
            fi
            validate_version "$2"
            update_version "$2"
            print_success "版本號已設置為: $2"
            ;;
        "increment")
            if [ -z "${2:-}" ]; then
                print_error "請指定版本增加類型"
                show_usage
                exit 1
            fi
            new_version=$(increment_version "$2")
            print_success "版本號已更新為: $new_version"
            ;;
        *)
            show_usage
            exit 1
            ;;
    esac
}

# 運行主函數
main "$@"