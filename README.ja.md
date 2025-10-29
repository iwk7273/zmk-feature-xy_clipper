# ZMK 機能: XY Clipper

このリポジトリは、トラックボールや光学センサーなどから得られる相対 XY 移動量をクリップするための ZMK 入力プロセッサモジュールです。

## 概要

`xy_clipper` 入力プロセッサはポインタ移動を安定化させるために設計されており、トラックボールによるスクロールなどの用途に適しています。

X/Y それぞれの移動を内部で蓄積し、`threshold`（閾値）を超えた時点で支配的な軸を判定します。既定では **Y 軸を優先** するため、縦方向のスクロール精度が向上します。蓄積された Y 軸移動が、X 軸移動の 2 倍より大きくない限り Y 軸が優先されます。

支配的な軸が決まると、もう一方の軸の移動量は 0 にされ、意図しない斜め移動を抑制します。

この挙動は以下の用途に有効です。
- 垂直または水平スクロールの安定化
- ポインタで直線を描く作業

## プロパティ

`xy_clipper` の挙動はデバイスツリーのプロパティで制御します。

- `threshold`（int、任意だが推奨）
  - **発火閾値**: 軸が出力イベントを発生させるまでに必要な移動量。
  - **感度調整**: 出力値は閾値で割られるため、高い値に設定すると感度が下がり（動きが遅く）なります。
- `invert-x`（boolean、任意）: X 軸の方向を反転する場合は `true`。
- `invert-y`（boolean、任意）: Y 軸の方向を反転する場合は `true`。

## 使い方

### 1. West マニフェストに追加

ファームウェアにこのモジュールを取り込むには、`config/west.yml` マニフェストにプロジェクトを追加します。フォークしている場合は `remote` や `revision` を自分のリポジトリに合わせて変更してください。

```yaml
manifest:
  remotes:
    - name: iwk7273
      url-base: https://github.com/iwk7273
  projects:
    - name: zmk-feature-xy_clipper
      remote: iwk7273
      revision: main
```

マニフェストを更新したら、モジュールを同期します。

- **GitHub Actions** でビルドしている場合: 変更をプッシュすると自動的に取得されます。
- **ローカルビルド** の場合: ZMK ファームウェアディレクトリで `west update` を実行します。
  ```sh
  west update
  ```

### 2. `prj.conf` で有効化

`prj.conf` に次の行を追加してください。

```conf
CONFIG_XY_CLIPPER=y
```

### 3. デバイスツリーオーバーレイで設定

使用するシールドの `.overlay` で `xy_clipper` プロセッサを定義し、入力リスナーに割り当てます。

```dts
/ {
    // 例: トラックボール用リスナーにアサイン
    trackball_listener {
        compatible = "zmk,input-listener";
        device = <&trackball>; // 利用する入力デバイス
        input-processors = <&xy_clipper>;
    };
};

/ {
    input_processors {
        xy_clipper: xy_clipper {
            compatible = "zmk,input-processor-xy-clipper";
            #input-processor-cells = <0>;

            // イベントを発生させるために必要な移動量。
            // 感度の除算にも使われる。値が大きいほど感度は低くなる。
            threshold = <20>;

            // 軸を反転する場合は 1、通常向きなら 0。
            invert-x = <0>;
            invert-y = <0>;
        };
    };
};
```
