pub mod csi {
    pub mod v1 {
        tonic::include_proto!("csi.v1");
    }
}

pub mod identity;
pub mod node;
