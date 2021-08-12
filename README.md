## 単体テスト

- 9軸センサ([KP-BMX055](https://prod.kyohritsu.com/KP-BMX055.html))
    - 上リンクにあるスケッチで動作確認。
    - Magのみ取得するスケッチが`unit_test/`中にあり。
    - 秋月のサイトのスケッチは[このサイト](https://analogicintelligence.blogspot.com/2020/01/)に解説がある通り間違いがいくつかあるので非推奨。
    - センサーをぐるぐる回しながらデータを取得し、データ点が円形に分布することを確認した。

- 超音波センサ([HC-SR04](https://akizukidenshi.com/catalog/g/gM-11009/))
    - `unit_test/`中のスケッチで動作確認。
    - 時々距離が1kmとかのとんでもない値となる。

- GPSセンサ([AE-GYSFDMAXB](https://akizukidenshi.com/catalog/g/gK-09991/))
    - `unit_test/`中のスケッチで動作確認(TX, RXは使わず別ピンでSoftwareSerialを使用)。
    - TinyGPS++ライブラリが必要。

- microSDカードスロット([AE-microSD-LLCNV](https://akizukidenshi.com/catalog/g/gK-14015/))
    - `ファイル > スケッチ例 > SD > CardInfo`のスケッチで動作確認。`chipSelect`を4から10に変更する必要がある。
    - 昨年の経験的に、ファイル名が長いと書き込めない。

- 照度センサ(CdSセル、型番不明)
    - `unit_test/`中のスケッチで動作確認。
    - 照度はデータシートの照度-抵抗値曲線とオームの法則から導出可能。

- カメラ([Groveシリアルカメラキット](https://jp.seeedstudio.com/Grove-Serial-Camera-Kit.html))
    - `unit_test/`中のスケッチで動作確認。最初に撮影しSDカードにデータを書き込んだ後は何もしない。

- 大気圧センサ([AE-MPL115A1](https://akizukidenshi.com/catalog/g/gI-06078/))
    - 上リンクの動作確認資料に従って動作確認。ライブラリのAE-MPL115A1.cppの`SPI_MPL115A1_CS`を9に変更する必要がある。
    - ２つある内片方は壊れている。

- 通信モジュール([XBee ZB S2C](https://akizukidenshi.com/catalog/g/gM-10072/))
    - `unit_test/`中のスケッチで動作確認。
    - MAC addressの0013A200に続く三文字が
        - 41A: 送信側。Arduinoと接続。
        - 418: 故障（XCTUが認識せず）。
        - 41C: 受信側。PCと変換基板、ケーブルを通して接続。
    - XCTUでのモジュール設定項目の内、`PAN ID`, `Destination Address High/Low`の設定が必要。
