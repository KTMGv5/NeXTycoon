#!/usr/bin/env python3
import json
import os
import sys
import urllib.request
import zipfile

def download_and_extract(url, target_dir):
    os.makedirs(target_dir, exist_ok=True)
    zip_path = os.path.join(target_dir, "temp_download.zip")
    print(f"Downloading {url} ...")
    req = urllib.request.Request(
        url,
        headers={"User-Agent": "NeXTycoon-AssetDownloader/1.0"}
    )
    with urllib.request.urlopen(req) as resp, open(zip_path, "wb") as out_file:
        out_file.write(resp.read())
    
    print(f"Extracting into {target_dir} ...")
    with zipfile.ZipFile(zip_path, "r") as zip_ref:
        zip_ref.extractall(target_dir)
    os.remove(zip_path)

def main():
    dest_dir = sys.argv[1] if len(sys.argv) > 1 else "data"
    dest_dir = os.path.abspath(dest_dir)
    os.makedirs(dest_dir, exist_ok=True)

    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    assets_file = os.path.join(root_dir, "assets.json")
    with open(assets_file, "r") as f:
        assets = json.load(f)

    # 1. Objects
    obj_dir = os.path.join(dest_dir, "object")
    if not os.path.isdir(obj_dir) or len(os.listdir(obj_dir)) < 100:
        download_and_extract(assets["objects"]["url"], obj_dir)

    # 2. Title sequences
    seq_dir = os.path.join(dest_dir, "sequence")
    if not os.path.isdir(seq_dir) or len(os.listdir(seq_dir)) < 2:
        download_and_extract(assets["title-sequences"]["url"], seq_dir)

    # 3. Asset packs (music & sfx)
    pack_dir = os.path.join(dest_dir, "assetpack")
    if not os.path.isdir(pack_dir) or len(os.listdir(pack_dir)) < 2:
        download_and_extract(assets["openmusic"]["url"], dest_dir)
        download_and_extract(assets["opensfx"]["url"], dest_dir)

    print(f"Game assets ready in {dest_dir}!")

if __name__ == "__main__":
    main()
