#[derive(Debug, serde::Serialize, serde::Deserialize)]
pub struct MetricsConfig {
    pub enabled: bool,
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
    #[serde(default)]
    pub metrics: bool,
    #[serde(default)]
    pub traces: bool,
    #[serde(default, rename = "faultyIO")]
    pub faulty_io: bool,
    #[serde(default, rename = "throttleIO")]
    pub throttle_io: bool,
    #[serde(default, rename = "fakeIO")]
    pub fake_io: bool,
}

impl Config {
    pub fn render(self, node_id: &str, otlp_endpoint: &str) -> FuseConfig {
        FuseConfig {
            metrics: MetricsConfig {
                enabled: self.metrics,
            },
            traces: TracesConfig {
                enabled: self.traces,
                nest_file_spans: true,
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
