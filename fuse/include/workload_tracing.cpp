#define FUSE_USE_VERSION 31

#include "otel.hpp"

#include <fuse_lowlevel.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include "opentelemetry/trace/span.h"
#include "opentelemetry/nostd/shared_ptr.h"

#include "workload_tracing.hpp"
#include "passthrough_hp.hpp"
#include <map>
#include "config_parser.hpp"

namespace ot		= opentelemetry;
namespace trace 	= ot::trace;
namespace nostd		= ot::nostd;

static fuse_lowlevel_ops *tracing_next;
static std::map<ino_t, nostd::shared_ptr<trace_api::Span>> fileSpans;

static bool nestFileSpans = false;
static std::string LIB_NAME = "fstracing";
static std::string SERVICE_NAME = "fs-workload-tracing";
static std::string HOST_NAME = "local-host";
static std::string END_PT = "localhost:4317";

static nostd::shared_ptr<trace_api::Span> getFileSpan(fuse_ino_t ino) {
	Inode& inode = get_inode(ino);
    ino_t inodeNum = inode.src_ino;

    // Add file span if it doesn't already exist
    if (fileSpans.find(inodeNum) == fileSpans.end()) {
        fileSpans.insert({
            inodeNum, 
            getSpan(LIB_NAME, "Inode " + std::to_string(inodeNum))
        });
    }
    return fileSpans[inodeNum];
}


// Helper functions to set attributes for spans

static void setAttribute(const nostd::shared_ptr<trace::Span>& span, fuse_req_t req) {
	auto ctxPtr = fuse_req_ctx(req);
	span->SetAttribute("User ID", ctxPtr->uid);
	span->SetAttribute("Group ID", ctxPtr->gid);
	span->SetAttribute("Process ID", ctxPtr->pid);
}

static void setAttribute(const nostd::shared_ptr<trace::Span>& span, fuse_ino_t ino, bool isParent) {
	Inode& inode = get_inode(ino);
	if (isParent) {
		span->SetAttribute("Parent Directory's Inode Number", inode.src_ino);	
	} else {
		span->SetAttribute("Inode Number", inode.src_ino);	
	}
}

// Low-level file operations

static void tracing_init(void *userdata, struct fuse_conn_info *conn)
{
    fileSpans.clear();
	initTracer(SERVICE_NAME, HOST_NAME, END_PT);
	auto span = getSpan(LIB_NAME, "Init");
	tracing_next->init(userdata, conn);
	span->End();
}

static void tracing_destroy(void *userdata) {
    for (const auto &fileSpan : fileSpans) {
        fileSpan.second->End();
    }
    fileSpans.clear();

    cleanupTracer();
	tracing_next->destroy(userdata);
}

static void tracing_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(parent);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Lookup");
	setAttribute(span, req);
	setAttribute(span, parent, true);
	span->SetAttribute("Name", name);
	tracing_next->lookup(req, parent, name);
	span->End();
}

static void tracing_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
    mode_t mode) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(parent);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Mkdir");
	setAttribute(span, req);
	setAttribute(span, parent, true);
	span->SetAttribute("Name", name);
	span->SetAttribute("Mode", mode);
	tracing_next->mkdir(req, parent, name, mode);
	span->End();
}

static void tracing_mknod(fuse_req_t req, fuse_ino_t parent, const char *name,
	mode_t mode, dev_t rdev) {
	std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(parent);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Mknod");
	setAttribute(span, req);
	setAttribute(span, parent, true);
	span->SetAttribute("Name", name);
	span->SetAttribute("Mode", mode);
	tracing_next->mknod(req, parent, name, mode, rdev);
	span->End();
}

static void tracing_symlink(fuse_req_t req, const char *link, fuse_ino_t parent,
	const char *name) {
	std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(parent);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Symlink");
	setAttribute(span, req);
	setAttribute(span, parent, true);
	span->SetAttribute("Name", name);
	span->SetAttribute("Link", link);
	tracing_next->symlink(req, link, parent, name);
	span->End();
}

static void tracing_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t parent,
	const char *name) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Link");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	setAttribute(span, parent, true);
	span->SetAttribute("Name", name);
	tracing_next->link(req, ino, parent, name);
	span->End();
}

static void tracing_unlink(fuse_req_t req, fuse_ino_t parent, const char *name) {
	std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(parent);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Unlink");
	setAttribute(span, req);
	setAttribute(span, parent, true);
	span->SetAttribute("Name", name);
	tracing_next->unlink(req, parent, name);
	span->End();
}

static void tracing_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name) {
	std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(parent);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }
	auto span = getSpan(LIB_NAME, "Rmdir");
	setAttribute(span, req);
	setAttribute(span, parent, true);
	span->SetAttribute("Name", name);
	tracing_next->rmdir(req, parent, name);
	span->End();
}

static void tracing_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
	fuse_ino_t newparent, const char *newname, unsigned int flags) {
	std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(parent);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Rename");
	setAttribute(span, req);
	span->SetAttribute("Flags", flags);

	setAttribute(span, parent, true);
	span->SetAttribute("Name", name);
	span->SetAttribute("New Parent Directory's Inode Number", get_inode(newparent).src_ino);
	span->SetAttribute("New Name", newname);

	tracing_next->rename(req, parent, name, newparent, newname, flags);
	span->End();
}

static void tracing_forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

    auto span = getSpan(LIB_NAME, "Forget");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	tracing_next->forget(req, ino, nlookup);
	span->End();
}

static void tracing_forget_multi(fuse_req_t req, size_t count,
	fuse_forget_data *forgets) { 
	auto span = getSpan(LIB_NAME, "Forget Multi");
	setAttribute(span, req);
	tracing_next->forget_multi(req, count, forgets);
	span->End();
}

static void tracing_getattr(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }


	auto span = getSpan(LIB_NAME, "Get Attribute");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	tracing_next->getattr(req, ino, fi);
	span->End();
}

static void tracing_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
	int valid, fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Set Attribute");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->SetAttribute("Valid", valid);
	tracing_next->setattr(req, ino, attr, valid, fi);
	span->End();
}

static void tracing_readlink(fuse_req_t req, fuse_ino_t ino) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Read Link");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	tracing_next->readlink(req, ino);
	span->End();
}

static void tracing_opendir(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Open Directory");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	tracing_next->opendir(req, ino, fi);
	span->End();
}

static void tracing_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
	off_t offset, fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Read Directory");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->SetAttribute("Size", size);
	span->SetAttribute("Offset", offset);
	tracing_next->readdir(req, ino, size, offset, fi);
	span->End();
}

static void tracing_readdirplus(fuse_req_t req, fuse_ino_t ino, size_t size,
	off_t offset, fuse_file_info *fi) {
    
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Read Directory Plus");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->SetAttribute("Size", size);
	span->SetAttribute("Offset", offset);
	tracing_next->readdirplus(req, ino, size, offset, fi);
	span->End();	
}

static void tracing_releasedir(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Release Directory");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	tracing_next->releasedir(req, ino, fi);
	span->End();	
}

static void tracing_fsyncdir(fuse_req_t req, fuse_ino_t ino, int datasync,
	fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Fsync Directory");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->SetAttribute("Datasync", datasync);
	tracing_next->fsyncdir(req, ino, datasync, fi);
	span->End();	
}

static void tracing_create(fuse_req_t req, fuse_ino_t parent, const char *name,
	mode_t mode, fuse_file_info *fi) {
	std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(parent);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }
	auto span = getSpan(LIB_NAME, "Create");
	setAttribute(span, req);
	setAttribute(span, parent, true);
	span->SetAttribute("Name", name);
	span->SetAttribute("Mode", mode);
	tracing_next->create(req, parent, name, mode, fi);
	span->End();	
}

static void tracing_open(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Open");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	tracing_next->open(req, ino, fi);
	span->End();
}

static void tracing_release(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Release");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	tracing_next->release(req, ino, fi);
	span->End();
}

static void tracing_flush(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Flush");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	tracing_next->flush(req, ino, fi);
	span->End();
}

static void tracing_fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
	fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Fsync");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->SetAttribute("Datasync", datasync);
	tracing_next->fsync(req, ino, datasync, fi);
	span->End();
}

static void tracing_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
	struct fuse_file_info *fi) 
{
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Read");	
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->SetAttribute("Size", size);
	span->SetAttribute("Offset", off);

	tracing_next->read(req, ino, size, off, fi);

	span->End();
}

static void tracing_write_buf(fuse_req_t req, fuse_ino_t ino, fuse_bufvec *in_buf,
                          off_t off, fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Write Buf");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->SetAttribute("Offset", off);
	tracing_next->write_buf(req, ino, in_buf, off, fi);
	span->End();
}

static void tracing_statfs(fuse_req_t req, fuse_ino_t ino) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Stat FS");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	tracing_next->statfs(req, ino);
	span->End();
}

#ifdef HAVE_POSIX_FALLOCATE
static void tracing_fallocate(fuse_req_t req, fuse_ino_t ino, int mode,
                          off_t offset, off_t length, fuse_file_info *fi) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Fallocate");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->SetAttribute("Offset", offset);
	span->SetAttribute("Length", length);
	span->SetAttribute("Mode", mode);
	tracing_next->fallocate(req, ino, mode, offset, length, fi);
	span->End();
}
#endif

static void tracing_flock(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi,
	int op) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Flock");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->SetAttribute("Locking Operation", op);
	tracing_next->flock(req, ino, fi, op);
	span->End();
}

#ifdef HAVE_SETXATTR
static void tracing_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
	const char *value, size_t size, int flags) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Set Extended Attribute");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->setAttribute("Name", name);
	span->SetAttribute("Value", value);
	span->SetAttribute("Size", size);
	span->SetAttribute("Flags", flags);
	tracing_next->setxattr(req, ino, name, value, size, flags);
	span->End();
}

static void tracing_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
	size_t size) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Get Extended Attribute");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->setAttribute("Name", name);
	span->SetAttribute("Size", size);
	tracing_next->getxattr(req, ino, name, size);
	span->End();
}

static void tracing_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "List Extended Attribute");
	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->SetAttribute("Size", size);
	tracing_next->listxattr(req, ino, size);
	span->End();
}

static void tracing_removexattr(fuse_req_t req, fuse_ino_t ino, const char *name) {
    std::unique_ptr<trace_api::Scope> scope = nullptr;
    nostd::shared_ptr<trace_api::Span> fileSpan;
    if (nestFileSpans) {
	    fileSpan = getFileSpan(ino);
        scope = std::make_unique<trace::Scope>(getScope(LIB_NAME, fileSpan));
    }

	auto span = getSpan(LIB_NAME, "Remove Extended Attribute");

	setAttribute(span, req);
	setAttribute(span, ino, false);
	span->setAttribute("Name", name);
	tracing_next->removexattr(req, ino, name);
	span->End();
}

#endif

fuse_lowlevel_ops tracing_operations(fuse_lowlevel_ops &next) {
    json config = getConfig("./config/config.json");
	if (config.contains("traces")) {
		auto configTraces = config["traces"];
		nestFileSpans = configTraces.contains("nestFileSpans") && configTraces["nestFileSpans"];
		if(configTraces.contains("otelLibName")) LIB_NAME = configTraces["otelLibName"];
		if(configTraces.contains("otelServiceName")) SERVICE_NAME = configTraces["otelServiceName"];
		if(configTraces.contains("otelHostName")) HOST_NAME = configTraces["otelHostName"];
		if(configTraces.contains("otelEndpt")) END_PT = configTraces["otelEndpt"];
	}

	tracing_next = &next;

	fuse_lowlevel_ops curr = next;
	curr.init = tracing_init;
	curr.destroy = tracing_destroy;
	curr.lookup = tracing_lookup;
	curr.mkdir = tracing_mkdir;
	curr.mknod = tracing_mknod;
	curr.symlink = tracing_symlink;
	curr.link = tracing_link;
	curr.unlink = tracing_unlink;
	curr.rmdir = tracing_rmdir;
	curr.rename = tracing_rename;
	curr.forget = tracing_forget;
	curr.forget_multi = tracing_forget_multi;
	curr.getattr = tracing_getattr;
	curr.setattr = tracing_setattr;
	curr.readlink = tracing_readlink;
	curr.opendir = tracing_opendir;
	curr.readdir = tracing_readdir;
	curr.readdirplus = tracing_readdirplus;
	curr.releasedir = tracing_releasedir;
	curr.fsyncdir = tracing_fsyncdir;
	curr.create = tracing_create;
	curr.open = tracing_open;
	curr.release = tracing_release;
	curr.flush = tracing_flush;
	curr.fsync = tracing_fsync;
	curr.read = tracing_read;
	curr.write_buf = tracing_write_buf;
	curr.statfs = tracing_statfs;
#ifdef HAVE_POSIX_FALLOCATE
    curr.fallocate = tracing_fallocate;
#endif
    curr.flock = tracing_flock;
#ifdef HAVE_SETXATTR
    curr.setxattr = tracing_setxattr;
    curr.getxattr = tracing_getxattr;
    curr.listxattr = tracing_listxattr;
    curr.removexattr = tracing_removexattr;
#endif
	
	return curr;
}
