#[derive(Debug, serde::Serialize, serde::Deserialize)]
pub struct Config {
    #[serde(default)]
    metrics: bool,
    #[serde(default)]
    traces: bool,
    #[serde(default, rename = "faultyIO")]
    faulty_io: bool,
    #[serde(default, rename = "throttleIO")]
    throttle_io: bool,
    #[serde(default, rename = "fakeIO")]
    fake_io: bool,
}
