use container_native_fs_interposer::{
    csi::v1::{identity_server::IdentityServer, node_server::NodeServer},
    identity::IdentityService,
    node::NodeService,
};
use std::{env, io::ErrorKind};
use tokio::net::UnixListener;
use tokio_stream::wrappers::UnixListenerStream;
use tonic::transport::Server;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let path = env::var("CSI_ENDPOINT")?;
    match std::fs::remove_file(&path) {
        Err(err) if err.kind() == ErrorKind::NotFound => (),
        result => result?,
    }
    Server::builder()
        .add_service(IdentityServer::new(IdentityService::new(&env::var(
            "CSI_NAME",
        )?)))
        .add_service(NodeServer::new(
            NodeService::new(&env::var("KUBE_NODE_NAME")?, &env::var("CSI_IMAGE")?).await,
        ))
        .serve_with_incoming(UnixListenerStream::new(UnixListener::bind(&path)?))
        .await?;
    Ok(())
}
