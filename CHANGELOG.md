## [1.10.2](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.10.1...v1.10.2) (2026-10-25)


### Bug Fixes

* **auth:** keep the cached session when restore runs before host selection (TASK-910) ([6279bd9](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/6279bd91318d9820dd13523ce71e3d695ce4dddd))
* **editor:** persist Import Config to DefaultGame.ini (TASK-910) ([6cac1a5](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/6cac1a53742759ffd5d3e00a31b99d93e7cea98c))
* **gift-codes:** send the player ID when validating a code ([4d0616b](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/4d0616bfa746459d376e6133475ecba843f71865))
* **plugin:** drop the baseline EngineVersion so UE 5.6+ loads without the incompatible prompt (TASK-910) ([945f543](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/945f54360afc77b632a04033d712bb5b6cc485a6))

## [1.10.1](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.10.0...v1.10.1) (2026-10-23)


### Bug Fixes

* include condensed JSON policy for game packaging ([#37](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/issues/37)) ([6d3dd1e](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/6d3dd1e0f0b83c2584d15f01e8d8de09bc38e12c))

# [1.10.0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.9.0...v1.10.0) (2026-10-23)


### Bug Fixes

* **chat:** preserve accepted sends while clearing the feed ([5cbdf37](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/5cbdf37727380d53dab1935f50bf2e1978cb8616))
* compile native Unreal groups test with strict Clang warnings ([556b1aa](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/556b1aa3d2864c9fd43efa38b3bfc5c17133137d))
* **docs:** align achievement and configurable group contracts ([967969a](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/967969af4e86ad45d1f31aee637e28d39aa48651))
* **gift-codes:** keep public validation independent of player sessions ([9f74063](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/9f740632ab9535b7e8ea78629f7bb29d9bce32f5))
* **gift-codes:** validate fresh sequence after typed redemption decode ([bee484d](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/bee484d02aac90905636d2b471e765b56a7c2c3e))
* **inbox:** compile native source helpers and example ([5cdb20e](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/5cdb20e7ac654c5d91170380a108c99d3cb4b829))
* **inbox:** retain legacy group acceptance references ([e450c77](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/e450c778d2ea8eec18495e21dec2fa220f18ce0a))
* **items:** preserve configurable quotas and retained inventory ([#27](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/issues/27)) ([dc0e561](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/dc0e5618113ce6c8c2f3a940d04a28203f6c6edc))
* **marketplace:** qualify native codec helpers and canonical wire choices ([20826cd](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/20826cdeeb501876b1e1d5012a49d8857d55e9f8))
* **marketplace:** use canonical lowercase Unreal UUIDs on the wire ([c24aee8](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/c24aee885d5d69a9ef64a46c2d143d0cc0817454))
* **news:** parse application/json array responses ([2de7c2f](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/2de7c2fd6740dd2f554bcf96438eac41d606ccb5))
* **player-values:** use engine integer parsing header path ([1d08cbd](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/1d08cbd045c2a3351bf7cd67722ce6df61bc0f9b))
* **player-values:** wait for native fixture requests through editor startup ([a8ce55c](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/a8ce55cab3ee679e90dea7585fa52bfcc991691b))
* prepare engine-specific Fab plugin packages ([#35](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/issues/35)) ([7426051](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/74260513cc049c70d997ae7a59bdcaedbb744c66))
* preserve configured limits and compact settlement originals ([3235079](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/32350792583ce08ee3cdbf88399a651583c94edb))
* preserve example identities across uncertain authentication ([b374f39](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/b374f399cbdec396e927a8d802359cce5f5fe773))
* **readme:** link the horizOn Discord with its real invite code (TASK-1046) ([26b52c9](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/26b52c9a377d2a7dd3b928aca53b6744862a0894))
* retain ranked source bindings and separate shared byte boundaries ([42d181e](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/42d181e6391e29c39c6f76e39660fa3c9da1d1a5))
* retain scoped match watermarks and parse received groups aggregates ([4927b31](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/4927b31dd902f3059979191bfca2abba841b9af3))
* **sdk:** keep one project key accessor after feature integration ([2e440da](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/2e440da104f393348decedb736fd60a89b8f979e))
* **sdk:** serialize generic and match-bound run starts ([7500879](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/7500879ca7d2f03d5facd8f43d41d4b80c4e1e23))
* **statistics:** add native Unreal batch manager and Blueprint nodes (TASK-1005) ([856e582](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/856e582b53b9877fbe34ead8940256a978242ba4))
* **statistics:** avoid macOS Boolean name collision ([e940b10](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/e940b10875b33cbdfe626cac7cd1e1bfc041c7a9))
* **statistics:** honor server configured catalog limits ([4862427](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/486242724a0c74229f631138cf261eb553beae09))
* support compact achievements and atomic reward invalidation ([0459fea](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/0459feaa6ccc508f5235a5d6e168adab29d80c9a))
* **transport:** fence retries across identity leave-and-return ([61b5974](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/61b5974cfe3b97158987bcee46a53ff511f58d89))
* **transport:** separate global rate retries from feature limits ([d040332](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/d040332d6e9a5d2cb9d2772f0579fbb4d92159df))
* **unreal:** avoid Player Values names colliding in unity builds ([0042a48](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/0042a488d9739b0180fd08ba46ae44c653758f64))


### Features

* **achievements:** add scoped snapshots and pending reward delivery ([bf0d81e](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/bf0d81ed820629d85341700642483e246db73279))
* add revisioned groups SDK and durable examples ([b0dad61](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/b0dad61162667a690852a21c7ff0f45f06007b75))
* **chat:** add authorized realtime feeds and durable original sends ([e866f69](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/e866f6902d3a92473271fc8f173d9a5ef8d24716))
* **chat:** persist device messages apart from live transport windows ([1ff70c8](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/1ff70c8650bef927020823f7e76de77a3c443ac9))
* **friends:** add explicit revision-bound native client ([b052196](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/b052196f701567685a556e0f550792bc9d941681))
* **gift-codes:** expose stable value redemption receipts ([3a9e687](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/3a9e6871eb4de06f49802c32039a400070761daa))
* **gift-codes:** prepare and persist exact V2 redemption intents ([4036482](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/40364829587c6681d0c52325a24560472dc76593))
* **inbox:** add typed Unreal inbox snapshots and safe claims ([4111ec7](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/4111ec78a048f41fd69114561fe52eb32a606333))
* **inbox:** expose typed source views and item return claims ([59024a8](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/59024a87b083a6b7dac2a1c4bf6d625faab56d8a))
* **items:** add typed Unreal inventory and durable action sample ([5203870](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/5203870f4592bc2399be06d173fb8ee318fa4b8b))
* **marketplace:** add native typed routes and durable original escrow sample ([0be836e](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/0be836e4be7da458eb4e675d2ce3443a0ce236be))
* **player-values:** add Unreal typed values and safe mutation handling ([b27bd00](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/b27bd00a9809e060205f571e3542337f27ed63cb))
* **realtime:** add native Unreal subscriptions and transport test (TASK-995) ([9535424](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/9535424a44ec771ddc550e857a06e4c468bb493d))
* **sdk:** add asynchronous matches and durable recovery ([c6ad688](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/c6ad6885aa84ae8d54e09e9a916994a3f617efd1))
* **sdk:** add shared objects with durable original intents ([3738ee3](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/3738ee3dea5d410369f4bcc7e66bc19db3a171b5))

# [1.9.0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.8.0...v1.9.0) (2026-10-11)


### Features

* **cloud-save:** revision-checked saves for several devices ([f65775f](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/f65775fd910246ab55c6d885081bb543ae2fe2cf))

# [1.8.0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.7.0...v1.8.0) (2026-10-02)


### Features

* **validated-actions:** optional run start context and sus result ([4d99257](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/4d992573e1af17b67905acb3fab4686cac201dac))

# [1.7.0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.6.0...v1.7.0) (2026-10-01)


### Bug Fixes

* **ci:** run transport tests on develop ([715a7e6](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/715a7e6389b090c0034e18556b9ebe9086050deb))
* **crash-reporting:** hash crash fingerprints with the SDK's own SHA-256 ([753ec4f](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/753ec4fb8b0d357d9634d3b9382bc1586a9fdd04))
* **gift-codes:** require a player session before redeeming a code ([a26b7e9](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/a26b7e9130dc870852b0fe94b9f297e2b382c1f3))
* **http:** clear error when still rate limited after the last retry (TASK-885) ([5bc5453](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/5bc5453e771b090f310f3adc103db8d4d877cc45))
* **sdk:** restore anonymous auth and POST binary cloud loads ([68002c3](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/68002c340a66a04d92fd7f3da51ffc0a0b257206))
* **security:** test signed leaderboard transport ([d48d138](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/d48d138cab12c95e75195e7968cb82ad12fdd312))
* **validated-actions:** keep the run on checks that run before the ticket is consumed ([eb4ce51](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/eb4ce51a09c4fff8261933d8ca21c28cb9ae1ede))
* **validated-actions:** report NOT_SUPPORTED for the simpleServer's generic 404 NOT_FOUND ([bd6a8dc](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/bd6a8dc3e315b93e398681ba3703fcc250f5b9bd))


### Features

* **player-profile:** add player profile manager, leaderboard profiles and granted unlocks ([0f160b4](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/0f160b499fe22d6fde233818e51a9f94d2cf79f3))
* **validated-actions:** add validated actions manager, run tickets and validated submit ([cc5fa1d](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/cc5fa1db15835e4fd8799c6d502ba38ae7d94beb))
* **validated-actions:** server-owned player state with GetState and cached state ([3965fd4](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/3965fd41aebaadcfc2d6fbdf04272fe19d9eaafc))
* **validated-actions:** upload the input log as evidence and report PLAYER_BANNED ([4f591e0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/4f591e03bd3d0a9ce477b063ec8662ff6d0fcc62))

# [1.6.0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.5.0...v1.6.0) (2026-06-24)


### Features

* **localization:** add HorizonLocalizationManager, async actions and example ([87e2433](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/87e2433edce0a87dcda98df815eb2c593efa20d2))

# [1.5.0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.4.1...v1.5.0) (2026-05-22)


### Features

* harmonize sdk multi-board APIs ([ee0fe62](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/ee0fe62de0bcb7fb7b979df1dc984f62daeb8445))

## [1.4.1](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.4.0...v1.4.1) (2026-05-14)


### Bug Fixes

* **sdk:** add per-feature minimal examples and Hello horizOn entry (TASK-217) ([abcb21b](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/abcb21b4755cfa7936b8239703a9cbe08de5995f))

# [1.4.0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.3.0...v1.4.0) (2026-04-20)


### Features

* **apple-signin:** apple sign-in support in unreal sdk ([bb8543f](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/bb8543f18fabe42028da53113b2b42d672db52b8))

# [1.3.0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.2.1...v1.3.0) (2026-04-11)


### Features

* add email sending manager with Blueprint async actions ([392745d](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/392745db7a6e65e288e887e10c270271d7aac5bb))

## [1.2.1](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.2.0...v1.2.1) (2026-02-23)


### Bug Fixes

* **ci:** trigger release build ([4e067ec](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/4e067ec8db98d80944cab80007f592ca60de70a3))

# [1.2.0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.1.0...v1.2.0) (2026-02-23)


### Features

* **crash:** add Blueprint async actions for crash reporting ([3c5a919](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/3c5a919bdff87896f771060ffaddec9513126519))
* **crash:** add Blueprint helper functions for crash reporting ([8560a61](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/8560a6179c0042b51f885b72374d9314d5fad282))
* **crash:** add CrashType enum, delegates, and CrashManager header ([1d960a3](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/1d960a3d4e013be03a95dd0a82e305be9d6372df))
* **crash:** implement CrashManager with full crash reporting logic ([d20638d](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/d20638d52f5ba95470dbf5b97496929f5ba3be55))
* **crash:** register CrashManager in subsystem with auto-breadcrumbs ([7251ed2](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/7251ed2db776e890bd617fb8e31eebdc81b0f782))

# [1.1.0](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/compare/v1.0.0...v1.1.0) (2026-02-22)


### Bug Fixes

* use AddUniqueDynamic for subsystem event handlers ([5bab6fc](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/5bab6fcea76a4b63adfa5daf9203f80ca1fe1a16))


### Features

* self-constructing demo UI for example widget ([b8c693a](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/b8c693ad8932c968da7e35c457dcdfefb0884170))

# 1.0.0 (2026-02-21)


### Bug Fixes

* resolve compilation issues from final review ([83cc639](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/83cc639839d034040cfb940f79210498aeeeaa69))


### Features

* add auth manager (signup, signin, session caching) ([701eb54](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/701eb54c9edfa1411204e0991b4942ad44adb237))
* add Blueprint async action nodes for all SDK operations ([d3ca32e](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/d3ca32e6cad481d9f8fbd657b84ae0d4c188fc24))
* add Blueprint function library for convenient access ([36d7eba](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/36d7ebacd3295e15f09494c050ce4bbe882e5a38))
* add cloud save manager (JSON and binary modes) ([8c722ae](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/8c722ae1bdfb1fc36e225449943fc93a2fdf021c))
* add data models (UserData, LeaderboardEntry, NewsEntry, NetworkResponse) ([e282b9b](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/e282b9b14cea2a1809d6cbec798e78f2a0728197))
* add editor config importer and Tools menu entries ([383e701](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/383e701717d19d09b6567be2ab7ee2ba182bade4))
* add editor dashboard widget with config display and connection test ([aa77972](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/aa77972a9cb9cb535bb7527d2c2598b79ab111db))
* add example widget base class for SDK demonstration ([8345ff9](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/8345ff948b8f5a4551964d65377f2871f4d6e8d6))
* add feedback manager with device info collection ([9dbe2b6](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/9dbe2b627ae5070d99c3d0bee5ab1d9ae8e5f976))
* add gift code manager (validate and redeem) ([3557c77](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/3557c77884f278050053a43ce9579e957b8f0474))
* add HTTP client with retry, rate-limiting, and host selection ([624b2ff](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/624b2ff72e30dbee257ae9cf874837951c8f7af2))
* add leaderboard manager with caching ([9f6a1d8](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/9f6a1d8e118a9166291caa9777df95c9fc2c4c2a))
* add news manager with 5-minute cache ([d362006](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/d36200687198d9682108dc0f3fe0322d58e983e1))
* add remote config manager with typed getters and caching ([5f81937](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/5f8193761abbc8355b127e2e73038cafd97d5b62))
* add session persistence via USaveGame ([8e49187](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/8e49187ea309da42d20aeeddc27068281e0a8039))
* add shared types, enums, and delegate declarations ([ac719ef](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/ac719ef421774105114cb39457ec14b35facbe6a))
* add UHorizonConfig developer settings ([90bedaa](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/90bedaae7e9842c911920e14384c5a6bedd92aca))
* add UHorizonSubsystem as main SDK entry point ([d78074f](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/d78074f43f0bb42bbd8c520e88df6e43180f2bee))
* add user log manager (server-side logging) ([9b49caa](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/9b49caa2158515a925554cd5256c6acbf2c38767))
* scaffold HorizonSDK plugin with runtime and editor modules ([ad33b1f](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/ad33b1fa820710087f2bf2742d5452469f9d82d3))
* wire all managers into subsystem initialization ([3afef33](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/commit/3afef33ec1c7d584fe5ccd8cc360d8aff56b7024))

# Changelog

All notable changes to the horizOn SDK for Unreal Engine will be documented in this file.

This changelog is automatically generated by [semantic-release](https://github.com/semantic-release/semantic-release).
