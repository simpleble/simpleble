use std::sync::{Mutex, PoisonError};

use futures::Stream;
use tokio::sync::mpsc;
use tokio_stream::wrappers::UnboundedReceiverStream;

// Sends each event to every open stream.
// NOTE: Each stream keeps its events until they are read, so that native threads never wait on a
// slow reader and no event is lost. Dropping a stream releases what it kept.
pub(crate) struct EventSender<T> {
    streams: Mutex<Vec<mpsc::UnboundedSender<T>>>,
}

impl<T: Clone> EventSender<T> {
    pub(crate) fn new() -> Self {
        Self {
            streams: Mutex::new(Vec::new()),
        }
    }

    pub(crate) fn subscribe(&self) -> impl Stream<Item = T> {
        let (sender, receiver) = mpsc::unbounded_channel();
        self.streams.lock().unwrap_or_else(PoisonError::into_inner).push(sender);

        UnboundedReceiverStream::new(receiver)
    }

    // Called from native threads, so it never panics. Streams that were dropped are removed.
    pub(crate) fn send(&self, event: T) {
        self.streams
            .lock()
            .unwrap_or_else(PoisonError::into_inner)
            .retain(|stream| stream.send(event.clone()).is_ok());
    }
}
