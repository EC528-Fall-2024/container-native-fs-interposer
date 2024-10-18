use crate::csi::v1::node_server::Node;
use crate::csi::v1::*;
use k8s_openapi::api::core::v1::{
    Container, HostPathVolumeSource, Pod, PodSpec, SecurityContext, Volume, VolumeMount,
};
use k8s_openapi::api::core::v1::{EnvVar, PersistentVolumeClaimVolumeSource};
use kube::runtime::conditions;
use kube::runtime::wait::await_condition;
use kube::{
    api::{ObjectMeta, PostParams},
    Api, Client,
};
use kube::{Resource, ResourceExt};
use nix::mount::MntFlags;
use std::env;
use std::{io::ErrorKind, path::Path};
use tonic::{Request, Response, Status};

pub struct NodeService {
    client: Client,
    node_id: String,
}

impl NodeService {
    pub async fn new(node_id: &str) -> Self {
        let client = Client::try_default().await.unwrap();
        Self {
            client,
            node_id: node_id.to_string(),
        }
    }
    pub fn new_interposer(
        &self,
        pod: &Pod,
        request: &NodePublishVolumeRequest,
    ) -> Result<Pod, Status> {
        let claim = request
            .volume_context
            .get("persistentVolumeClaimName")
            .ok_or(Status::invalid_argument(
                "missing persistentVolumeClaimName in volumeAttributes",
            ))?;

        let command = request
            .volume_context
            .get("command")
            .ok_or(Status::invalid_argument(
                "missing command in volumeAttributes",
            ))?;

        let command = shlex::split(command).ok_or(Status::invalid_argument(
            "unable to split command in volumeAttributes",
        ))?;

        let source_path = "/lowerdir";

        Ok(Pod {
            metadata: ObjectMeta {
                name: Some(format!("{}-{}", pod.name_unchecked(), request.volume_id)),
                namespace: pod.namespace(),
                owner_references: Some(pod.owner_ref(&()).into_iter().collect()),
                ..Default::default()
            },
            spec: Some(PodSpec {
                node_selector: Some(
                    [(
                        "kubernetes.io/hostname".to_string(),
                        // FIXME: hostname might not equal node name
                        env::var("KUBE_NODE_NAME").unwrap().to_string(),
                    )]
                    .into(),
                ),
                restart_policy: Some("Never".to_string()),
                containers: vec![Container {
                    command: Some(command),
                    env: Some(vec![
                        EnvVar {
                            name: "SOURCE_PATH".to_string(),
                            value: Some(source_path.to_string()),
                            ..Default::default()
                        },
                        EnvVar {
                            name: "TARGET_PATH".to_string(),
                            value: Some(request.target_path.clone()),
                            ..Default::default()
                        },
                    ]),
                    image: Some("docker.io/library/csi-node:latest".to_string()),
                    image_pull_policy: Some("IfNotPresent".to_string()),
                    name: "interposer".to_string(),
                    security_context: Some(SecurityContext {
                        privileged: Some(true),
                        ..Default::default()
                    }),
                    volume_mounts: Some(vec![
                        VolumeMount {
                            mount_path: "/var/lib/kubelet/pods".to_string(),
                            mount_propagation: Some("Bidirectional".to_string()),
                            name: "mountpoint-dir".to_string(),
                            ..Default::default()
                        },
                        VolumeMount {
                            mount_path: "/dev".to_string(),
                            name: "dev-dir".to_string(),
                            ..Default::default()
                        },
                        VolumeMount {
                            mount_path: source_path.to_string(),
                            name: "lowerdir".to_string(),
                            ..Default::default()
                        },
                    ]),
                    ..Default::default()
                }],
                volumes: Some(vec![
                    Volume {
                        host_path: Some(HostPathVolumeSource {
                            path: "/var/lib/kubelet/pods".to_string(),
                            type_: Some("DirectoryOrCreate".to_string()),
                        }),
                        name: "mountpoint-dir".to_string(),
                        ..Default::default()
                    },
                    Volume {
                        host_path: Some(HostPathVolumeSource {
                            path: "/dev".to_string(),
                            type_: Some("Directory".to_string()),
                        }),
                        name: "dev-dir".to_string(),
                        ..Default::default()
                    },
                    Volume {
                        persistent_volume_claim: Some(PersistentVolumeClaimVolumeSource {
                            claim_name: claim.to_string(),
                            ..Default::default()
                        }),
                        name: "lowerdir".to_string(),
                        ..Default::default()
                    },
                ]),
                ..Default::default()
            }),
            ..Default::default()
        })
    }
}

#[tonic::async_trait]
impl Node for NodeService {
    async fn node_stage_volume(
        &self,
        _: Request<NodeStageVolumeRequest>,
    ) -> Result<Response<NodeStageVolumeResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_unstage_volume(
        &self,
        _: Request<NodeUnstageVolumeRequest>,
    ) -> Result<Response<NodeUnstageVolumeResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_publish_volume(
        &self,
        request: Request<NodePublishVolumeRequest>,
    ) -> Result<Response<NodePublishVolumeResponse>, Status> {
        let request = request.into_inner();

        let pod_namespace = request
            .volume_context
            .get("csi.storage.k8s.io/pod.namespace")
            .unwrap();
        let pod_name = request
            .volume_context
            .get("csi.storage.k8s.io/pod.name")
            .unwrap();

        let pods: Api<Pod> = Api::namespaced(self.client.clone(), pod_namespace);

        let pod = pods.get(pod_name).await.unwrap();

        match std::fs::create_dir(&request.target_path) {
            Err(err) if err.kind() == ErrorKind::AlreadyExists => (),
            result => result?,
        };

        let interposer_pod = pods
            .create(
                &PostParams::default(),
                &self.new_interposer(&pod, &request)?,
            )
            .await
            .unwrap();

        await_condition(
            pods,
            &interposer_pod.name_unchecked(),
            conditions::is_pod_running(),
        )
        .await
        .unwrap();

        Ok(Response::new(NodePublishVolumeResponse {}))
    }
    async fn node_unpublish_volume(
        &self,
        request: Request<NodeUnpublishVolumeRequest>,
    ) -> Result<Response<NodeUnpublishVolumeResponse>, Status> {
        let request = request.into_inner();

        // FIXME: cleanup the fuse process
        nix::mount::umount2(Path::new(&request.target_path), MntFlags::empty())
            .map_err(|err| Status::internal(err.to_string()))?;

        std::fs::remove_dir_all(request.target_path)?;
        Ok(Response::new(NodeUnpublishVolumeResponse {}))
    }
    async fn node_get_volume_stats(
        &self,
        _: Request<NodeGetVolumeStatsRequest>,
    ) -> Result<Response<NodeGetVolumeStatsResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_expand_volume(
        &self,
        _: Request<NodeExpandVolumeRequest>,
    ) -> Result<Response<NodeExpandVolumeResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_get_capabilities(
        &self,
        _: Request<NodeGetCapabilitiesRequest>,
    ) -> Result<Response<NodeGetCapabilitiesResponse>, Status> {
        Ok(Response::new(NodeGetCapabilitiesResponse {
            capabilities: vec![],
        }))
    }
    async fn node_get_info(
        &self,
        _: Request<NodeGetInfoRequest>,
    ) -> Result<Response<NodeGetInfoResponse>, Status> {
        Ok(Response::new(NodeGetInfoResponse {
            node_id: self.node_id.clone(),
            max_volumes_per_node: 0,
            accessible_topology: None,
        }))
    }
}
