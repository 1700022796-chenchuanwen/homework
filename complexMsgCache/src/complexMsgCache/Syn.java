package complexMsgCache;

public class Syn {
	int count =0;
	public Syn(int i) {
		count=i;
	}

	
	public synchronized void Wait() {
		count--;
		if(count<0) {
			try {
				this.wait();
			}catch(InterruptedException e) {
				e.printStackTrace();
			}
		}
	}
	public synchronized void Signal() {
		count++;
		if(count<=0) {
			this.notify();
		}
	}
	
}
