use crate::csi::v1::node_server::Node;
use crate::csi::v1::*;
use k8s_openapi::api::core::v1::Pod;
use kube::{Api, Client};
use nix::mount::MntFlags;
use std::{io::ErrorKind, path::Path, process::Command};
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

        let pods: Api<Pod> = Api::namespaced(self.client.clone(), &pod_namespace);
        let _ = pods.get(&pod_name).await;

        match std::fs::create_dir(&request.target_path) {
            Err(err) if err.kind() == ErrorKind::AlreadyExists => (),
            result => result?,
        };

        // FIXME: check if a filesystem is already mounted
        Command::new("basic_passthrough")
            .args([&request.target_path])
            .spawn()?;

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
