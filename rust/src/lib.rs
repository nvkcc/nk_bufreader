//! For algorithmic brainstorming/modelling.

struct Reader<const N: usize> {
    buffer: [u8; N],
    left: u8,
    right: Option<u8>,
    /// The first invalid value. Useful for pointer for read() calls.
    end: u8,
}

impl<const N: usize> Reader<N> {
    pub fn new() -> Self {
        Self {
            buffer: [0; N],
            left: 0,
            right: None,
            end: 0,
        }
    }
}

#[test]
fn empty_string() {
    let mut r = Reader::<8> { buffer: [0; 8] };
}
