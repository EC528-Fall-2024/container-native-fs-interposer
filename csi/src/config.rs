use serde::Deserialize;
use std::str::FromStr;

#[derive(Debug, serde::Serialize, serde::Deserialize)]
pub struct MetricsConfig {
    pub enabled: bool,
    #[serde(rename = "readCounter")]
    pub read_counter: bool,
    #[serde(rename = "writeCounter")]
    pub write_counter: bool,
    #[serde(rename = "readLatencyHist")]
    pub read_latency_hist: bool,
    #[serde(rename = "writeLatencyHist")]
    pub write_latency_hist: bool,
    #[serde(rename = "dirCounter")]
    pub dir_counter: bool,
}

#[derive(Debug, serde::Serialize, serde::Deserialize)]
pub struct TracesConfig {
    pub enabled: bool,
    #[serde(rename = "nestFileSpans")]
    pub nest_file_spans: bool,
    #[serde(rename = "otelLibName")]
    pub otel_lib_name: String,
    #[serde(rename = "otelServiceName")]
    pub otel_service_name: String,
    #[serde(rename = "otelHostName")]
    pub otel_host_name: String,
    #[serde(rename = "otelEndpt")]
    pub otel_endpoint: String,
}

#[derive(Debug, serde::Serialize, serde::Deserialize)]
pub struct FaultyIOConfig {
    pub enabled: bool,
}

#[derive(Debug, serde::Serialize, serde::Deserialize)]
pub struct ThrottleIOConfig {
    pub enabled: bool,
}

#[derive(Debug, serde::Serialize, serde::Deserialize)]
pub struct FuseConfig {
    pub metrics: MetricsConfig,
    pub traces: TracesConfig,
    #[serde(rename = "faultyIO")]
    pub faulty_io: FaultyIOConfig,
    #[serde(rename = "throttleIO")]
    pub throttle_io: ThrottleIOConfig,
}

#[derive(Debug, serde::Serialize, serde::Deserialize)]
pub struct Config {
    #[serde(default, deserialize_with = "bool_str")]
    pub metrics: bool,
    #[serde(default, deserialize_with = "bool_str")]
    pub traces: bool,
    #[serde(default, rename = "tracesNested", deserialize_with = "bool_str")]
    pub traces_nested: bool,
    #[serde(default, rename = "faultyIO", deserialize_with = "bool_str")]
    pub faulty_io: bool,
    #[serde(default, rename = "throttleIO", deserialize_with = "bool_str")]
    pub throttle_io: bool,
    #[serde(default, rename = "fakeIO", deserialize_with = "bool_str")]
    pub fake_io: bool,
}

impl Config {
    pub fn render(self, node_id: &str, otlp_endpoint: &str) -> FuseConfig {
        FuseConfig {
            metrics: MetricsConfig {
                enabled: self.metrics,
                read_counter: true,
                write_counter: true,
                read_latency_hist: true,
                write_latency_hist: true,
                dir_counter: true,
            },
            traces: TracesConfig {
                enabled: self.traces,
                nest_file_spans: self.traces_nested,
                otel_lib_name: "csi-interposer".to_string(),
                otel_service_name: "traces".to_string(),
                otel_host_name: node_id.to_string(),
                otel_endpoint: otlp_endpoint.to_string(),
            },
            faulty_io: FaultyIOConfig {
                enabled: self.faulty_io,
            },
            throttle_io: ThrottleIOConfig {
                enabled: self.throttle_io,
            },
        }
    }
}

fn bool_str<'de, D>(deserializer: D) -> Result<bool, D::Error>
where
    D: serde::de::Deserializer<'de>,
{
    bool::from_str(&String::deserialize(deserializer)?).map_err(serde::de::Error::custom)
}
